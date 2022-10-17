#include "model.hpp"

#include "tiny_gltf.h"

#include "game.hpp"
#include "matrix_math.hpp"
#include "mesh_asset.hpp"
#include "texture_asset.hpp"
#include "lib_ext/imgui_extra.hpp"

#include "renderer/renderer.hpp"
#include "renderer/renderable.hpp"
#include "renderer/render_scene.hpp"
#include "renderer/assets/mesh.hpp"
#include "renderer/assets/material.hpp"

#include "game/asset_browser.hpp"

namespace spellbook {

vector<ModelCPU> ModelCPU::split() {
    auto traverse = [](ModelCPU& model, id_ptr<ModelCPU::Node> node, auto&& traverse) -> void {
        for (id_ptr<Node> child : node->children) {
            model.nodes.insert_back(child);
            traverse(model, child, traverse);
        }
    };

    vector<ModelCPU> models;
    models.resize(root_node->children.size());
    u32 i = 0;
    for (id_ptr<Node> child : root_node->children) {
        child->parent = id_ptr<Node>::null();
        traverse(models[i++], child, traverse);
    }

    return models;
}

void inspect(ModelCPU* model) {
    if (model == nullptr) {
        ImGui::Text("No model");
        return;
    }
    ImGui::Text("file_name: %s", model->file_name.c_str());

    std::function<void(id_ptr<ModelCPU::Node>)> traverse;
    traverse = [&traverse](id_ptr<ModelCPU::Node> node_ptr) {
        ImGui::PushID(node_ptr.value);
        auto& node = *node_ptr;
        ImGui::Text("name:            %s", node.name.c_str());
        ImGui::Text("mesh_asset_path: %s", node.mesh_asset_path.c_str());
        ImGui::Text("mat_asset_path:  %s", node.material_asset_path.c_str());
        DragMat4("Transform", &node.transform, 0.01f, "%.2f");

        ImGui::Text("Children");
        for (auto& child : node.children) {
            if (child.valid() && ImGui::TreeNode(child->name.c_str())) {
                traverse(child);
                ImGui::TreePop();
            }
        }
        ImGui::Separator();
        ImGui::PopID();
    };
    if (model->root_node.valid() && ImGui::TreeNode(model->root_node->name.c_str())) {
        traverse(model->root_node);
        ImGui::TreePop();
    } else if (!model->root_node.valid()) {
        ImGui::Text("No root node");
    }
}


void save_model(const ModelCPU& model) {
    json j;
    j["root_node"] = make_shared<json_value>(to_jv(model.root_node));

    vector<json_value> json_nodes;
    for (id_ptr<ModelCPU::Node> node : model.nodes) {
        json_nodes.insert_back(to_jv_full(node));
    }
    j["nodes"] = make_shared<json_value>(to_jv(json_nodes));
    
    string ext = std::filesystem::path(model.file_name).extension().string();
    assert_else(ext == model_extension);
    
    file_dump(j, get_resource_path(model.file_name));
}

fs::path _convert_to_relative(const fs::path& path) {
    return path.lexically_proximate(game.resource_folder);
}

ModelCPU load_model(const fs::path& _input_path) {
    fs::path input_path = _input_path.string().starts_with("resources") ?
        _input_path.lexically_proximate(game.resource_folder) : _input_path;
    
    
    string ext = input_path.extension().string();
    warn_else(ext == model_extension)
        return {};
    
    ModelCPU model;
    json      j = parse_file(get_resource_path(input_path.string()));
    model.file_name = input_path.string();
    if (j.contains("root_node"))
        model.root_node = from_jv<id_ptr<ModelCPU::Node>>(*j["root_node"]);

    if (j.contains("nodes")) {
        for (const json_value& jv : j["nodes"]->get_list()) {
            id_ptr<ModelCPU::Node> node = from_jv_full(jv, (id_ptr<ModelCPU::Node>*) 0);
            model.nodes.insert_back(node);
        }
    }
    return model;
}

ModelGPU instance_model(RenderScene& render_scene, const ModelCPU& model) {
    ModelGPU model_gpu;

    for (id_ptr<ModelCPU::Node> node_ptr : model.nodes) {
        const ModelCPU::Node& node           = *node_ptr;
        Renderable             renderable     = Renderable(node.mesh_asset_path, node.material_asset_path, node.transform);
        auto                   new_renderable = render_scene.add_renderable(renderable);
        model_gpu.renderables.insert_back(new_renderable);
    }

    return model_gpu;
}

void deinstance_model(RenderScene& render_scene, const ModelGPU& model) {
    for (auto& r_slot : model.renderables) {
        render_scene.renderables.remove_element(r_slot);
    }
}

m44 ModelCPU::Node::calculate_transform() const {
    return !parent.valid() ? transform : parent->calculate_transform() * transform;
}

constexpr m44 gltf_fixup = m44(0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1);

void _unpack_gltf_buffer(tinygltf::Model& model, tinygltf::Accessor& accessor, vector<u8>& output_buffer);
void _extract_gltf_vertices(tinygltf::Primitive& primitive, tinygltf::Model& model, vector<Vertex>& vertices);
void _extract_gltf_indices(tinygltf::Primitive& primitive, tinygltf::Model& model, vector<u32>& indices);
string _calculate_gltf_mesh_name(tinygltf::Model& model, int mesh_index, int primitive_index);
string _calculate_gltf_material_name(tinygltf::Model& model, int material_index);
bool _convert_gltf_meshes(tinygltf::Model& model, const fs::path& output_folder);
bool _convert_gltf_materials(tinygltf::Model& model, const fs::path& output_folder);
m44 _calculate_matrix(tinygltf::Node& node);

ModelCPU convert_to_model(const fs::path& input_path, const fs::path& output_folder, const fs::path& output_name) {
    // TODO: gltf can't support null materials
    fs::create_directory(game.resource_folder);
    auto folder = game.resource_folder / output_folder;
    fs::create_directory(folder);

    const auto& ext = input_path.extension().string();
    assert_else(possible_model_asset(input_path))
        return {};
    tinygltf::Model    gltf_model;
    tinygltf::TinyGLTF loader;
    string             err, warn;

    bool ret = ext == ".gltf"
                   ? loader.LoadASCIIFromFile(&gltf_model, &err, &warn, input_path.string())
                   : loader.LoadBinaryFromFile(&gltf_model, &err, &warn, input_path.string());

    if (!warn.empty())
        console_error(fmt_("Conversion warning while loading \"{}\": {}", input_path.string(), warn), "asset.import", ErrorType_Warning);
    if (!err.empty())
        console_error(fmt_("Conversion error while loading \"{}\": {}", input_path.string(), err), "asset.import", ErrorType_Severe);
    assert_else(ret)
        return {};

    _convert_gltf_meshes(gltf_model, output_folder);
    _convert_gltf_materials(gltf_model, output_folder);
    
    ModelCPU model_cpu;

    // calculate parent hierarchies
    for (u32 i = 0; i < gltf_model.nodes.size(); i++) {
        model_cpu.nodes.insert_back(id_ptr<ModelCPU::Node>::emplace());
    }
    for (u32 i = 0; i < gltf_model.nodes.size(); i++) {
        auto& children = model_cpu.nodes[i]->children;
        for (u32 c : gltf_model.nodes[i].children) {
            children.insert_back(model_cpu.nodes[c]);
            model_cpu.nodes[c]->parent = model_cpu.nodes[i];
        }
    }

    bool multiple_valid = false;
    for (auto& node : model_cpu.nodes) {
        if (!node->parent.valid()) {
            // If we've already set a root node
            multiple_valid = model_cpu.root_node != id_ptr<ModelCPU::Node>::null();
            model_cpu.root_node = node;
        }
    }

    if (multiple_valid) {
        auto root_node = id_ptr<ModelCPU::Node>::emplace();
        for (auto& node : model_cpu.nodes) {
            if (!node->parent.valid()) {
                node->parent = root_node;
                root_node->children.insert_back(node);
            }
        }
        model_cpu.root_node = root_node;
        model_cpu.nodes.insert_back(root_node);
        root_node->name = "root_node";
        root_node->transform = m44::identity();
    }

    vector<u32> multimat_nodes;
    for (u32 i = 0; i < gltf_model.nodes.size(); i++) {
        auto& gltf_node = gltf_model.nodes[i];
        auto& model_node = *model_cpu.nodes[i];

        if (gltf_node.mesh >= 0 && gltf_model.meshes[gltf_node.mesh].primitives.size() == 1) {
            auto mesh = gltf_model.meshes[gltf_node.mesh];

            auto primitive = mesh.primitives[0];
            int  material  = primitive.material;
            assert_else(material >= 0 && "Need material on node, default mat NYI");
            string mesh_name     = _calculate_gltf_mesh_name(gltf_model, gltf_node.mesh, 0);
            string material_name = _calculate_gltf_material_name(gltf_model, material);

            fs::path mesh_path     = output_folder / (mesh_name + mesh_extension);
            fs::path material_path = output_folder / (material_name + material_extension);

            model_node.name                = gltf_node.name;
            model_node.mesh_asset_path     = mesh_path.string();
            model_node.material_asset_path = material_path.string();
            model_node.transform           = _calculate_matrix(gltf_node);
        }
        // Serving hierarchy
        else {
            model_node.name      = gltf_node.name;
            model_node.transform = _calculate_matrix(gltf_node);

            if (gltf_node.mesh >= 0)
                multimat_nodes.insert_back(i);
        }
    }
    
    model_cpu.root_node->transform = gltf_fixup * model_cpu.root_node->transform;

    // iterate nodes with multiple materials, convert each submesh into a node
    for (u32 i = 0; i < multimat_nodes.size(); i++) {
        auto  multimat_node_index = multimat_nodes[i];
        auto& multimat_node       = gltf_model.nodes[multimat_node_index];

        if (multimat_node.mesh < 0)
            break;

        auto mesh = gltf_model.meshes[multimat_node.mesh];

        for (u32 i_primitive = 0; i_primitive < mesh.primitives.size(); i_primitive++) {
            auto                    primitive = mesh.primitives[i_primitive];
            id_ptr<ModelCPU::Node> model_node_ptr      = id_ptr<ModelCPU::Node>::emplace();
            model_cpu.nodes.insert_back(model_node_ptr);

            char buffer[50];

            itoa(i_primitive, buffer, 10);

            int    material      = primitive.material;
            string material_name = _calculate_gltf_material_name(gltf_model, material);
            string mesh_name     = _calculate_gltf_mesh_name(gltf_model, multimat_node.mesh, i_primitive);

            fs::path material_path = output_folder / (material_name + material_extension);
            fs::path mesh_path     = output_folder / (mesh_name + mesh_extension);

            model_node_ptr->name      = model_cpu.nodes[multimat_node_index]->name + "_PRIM_" + &buffer[0];
            model_node_ptr->mesh_asset_path      = mesh_path.string();
            model_node_ptr->material_asset_path  = material_path.string();
            model_node_ptr->transform = m44::identity();
            model_node_ptr->parent    = model_cpu.nodes[multimat_node_index];
            model_cpu.nodes[multimat_node_index]->children.insert_back(model_node_ptr);
        }
    }

    fs::path scene_path = output_folder / output_name;
    scene_path.replace_extension(model_extension);

    model_cpu.file_name = scene_path.string();

    return model_cpu;
}













void _unpack_gltf_buffer(tinygltf::Model& model, tinygltf::Accessor& accessor, vector<u8>& output_buffer) {
    int                   buffer_id     = accessor.bufferView;
    u64                   element_bsize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
    tinygltf::BufferView& buffer_view   = model.bufferViews[buffer_id];
    tinygltf::Buffer&     buffer_data   = model.buffers[buffer_view.buffer];
    u8*                   dataptr       = buffer_data.data.data() + accessor.byteOffset + buffer_view.byteOffset;
    int                   components    = tinygltf::GetNumComponentsInType(accessor.type);
    element_bsize *= components;
    u64 stride = buffer_view.byteStride;
    if (stride == 0) {
        stride = element_bsize;
    }

    output_buffer.resize(accessor.count * element_bsize);

    for (u32 i = 0; i < accessor.count; i++) {
        u8* dataindex = dataptr + stride * i;
        u8* targetptr = output_buffer.data() + element_bsize * i;

        memcpy(targetptr, dataindex, element_bsize);
    }
}

void _extract_gltf_vertices(tinygltf::Primitive& primitive, tinygltf::Model& model, vector<Vertex>& vertices) {
    tinygltf::Accessor& pos_accessor = model.accessors[primitive.attributes["POSITION"]];
    vertices.resize(pos_accessor.count);
    vector<u8> pos_data;
    _unpack_gltf_buffer(model, pos_accessor, pos_data);

    for (int i = 0; i < vertices.size(); i++) {
        if (pos_accessor.type == TINYGLTF_TYPE_VEC3) {
            if (pos_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                float* dtf = (float*) pos_data.data();

                vertices[i].position.x = *(dtf + (i * 3) + X);
                vertices[i].position.y = *(dtf + (i * 3) + Y);
                vertices[i].position.z = *(dtf + (i * 3) + Z);
            } else {
                assert_else(false);
            }
        } else {
            assert_else(false);
        }
    }

    tinygltf::Accessor& normal_accessor = model.accessors[primitive.attributes["NORMAL"]];
    vector<u8>          normal_data;
    _unpack_gltf_buffer(model, normal_accessor, normal_data);
    for (int i = 0; i < vertices.size(); i++) {
        if (normal_accessor.type == TINYGLTF_TYPE_VEC3) {
            if (normal_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                float* dtf = (float*) normal_data.data();

                vertices[i].normal[X] = *(dtf + (i * 3) + X);
                vertices[i].normal[Y] = *(dtf + (i * 3) + Y);
                vertices[i].normal[Z] = *(dtf + (i * 3) + Z);
            } else {
                assert_else(false);
            }
        } else {
            assert_else(false);
        }
    }

    tinygltf::Accessor& uv_accessor = model.accessors[primitive.attributes["TEXCOORD_0"]];
    vector<u8>          uv_data;
    _unpack_gltf_buffer(model, uv_accessor, uv_data);
    for (int i = 0; i < vertices.size(); i++) {
        if (uv_accessor.type == TINYGLTF_TYPE_VEC2) {
            if (uv_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                float* dtf = (float*) uv_data.data();

                // vec3f
                vertices[i].uv[0] = *(dtf + (i * 2) + 0);
                vertices[i].uv[1] = *(dtf + (i * 2) + 1);
            } else {
                assert_else(false && "Only FLOAT supported for UV coordinate in GLTF convert");
            }
        } else if (uv_accessor.type == TINYGLTF_TYPE_VEC3) {
            if (uv_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                float* dtf = (float*) uv_data.data();

                // vec3f
                vertices[i].uv[0] = *(dtf + (i * 3) + 0);
                vertices[i].uv[1] = *(dtf + (i * 3) + 1);
            } else {
                assert_else(false && "Only FLOAT supported for UV coordinate in GLTF convert");
            }
        } else {
            assert_else(false && "Unsupported type for UV coordinate in GLTF convert");
        }
    }

    if (primitive.attributes["COLOR"] != 0) {
        tinygltf::Accessor& color_accessor = model.accessors[primitive.attributes["COLOR"]];
        vector<u8>          color_data;
        _unpack_gltf_buffer(model, color_accessor, color_data);
        for (int i = 0; i < vertices.size(); i++) {
            if (color_accessor.type == TINYGLTF_TYPE_VEC3) {
                if (color_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                    float* dtf = (float*) color_data.data();

                    vertices[i].color[0] = *(dtf + (i * 3) + 0);
                    vertices[i].color[1] = *(dtf + (i * 3) + 1);
                    vertices[i].color[2] = *(dtf + (i * 3) + 2);
                } else {
                    assert_else(false);
                }
            } else {
                assert_else(false);
            }
        }
    }
}

void _extract_gltf_indices(tinygltf::Primitive& primitive, tinygltf::Model& model, vector<u32>& indices) {
    int index_accessor = primitive.indices;

    int    index_buffer   = model.accessors[index_accessor].bufferView;
    int    component_type = model.accessors[index_accessor].componentType;
    size_t indexsize      = tinygltf::GetComponentSizeInBytes(component_type);

    tinygltf::BufferView& indexview = model.bufferViews[index_buffer];
    int                   bufferidx = indexview.buffer;

    tinygltf::Buffer& buffindex = (model.buffers[bufferidx]);
    u8*               dataptr   = buffindex.data.data() + indexview.byteOffset;
    vector<u8>        unpackedIndices;
    _unpack_gltf_buffer(model, model.accessors[index_accessor], unpackedIndices);
    for (int i = 0; i < model.accessors[index_accessor].count; i++) {
        u32 index;
        switch (component_type) {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                auto bfr = (u16*) unpackedIndices.data();
                index    = *(bfr + i);
            }
            break;
            case TINYGLTF_COMPONENT_TYPE_SHORT: {
                auto bfr = (s16*) unpackedIndices.data();
                index    = *(bfr + i);
            }
            break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                auto bfr = (u32*) unpackedIndices.data();
                index    = *(bfr + i);
            }
            break;
            case TINYGLTF_COMPONENT_TYPE_INT: {
                s32* bfr = (s32*) unpackedIndices.data();
                index    = *(bfr + i);
            }
            break;
            default:
                assert_else(false && "Only SHORT/USHORT supported for index in GLTF convert");
        }

        indices.insert_back(index);
    }

    for (u32 i = 0; i < indices.size() / 3; i++) {
        // flip the triangle
        std::swap(indices[i * 3 + 1], indices[i * 3 + 2]);
    }
}

string _calculate_gltf_mesh_name(tinygltf::Model& model, int mesh_index, int primitive_index) {
    char buffer0[50];
    char buffer1[50];
    itoa(mesh_index, buffer0, 10);
    itoa(primitive_index, buffer1, 10);

    string meshname = "MESH_" + string{&buffer0[0]} + "_" + model.meshes[mesh_index].name;

    bool multiprim = model.meshes[mesh_index].primitives.size() > 1;
    if (multiprim) {
        meshname += "_PRIM_" + string{&buffer1[0]};
    }

    return meshname;
}

string _calculate_gltf_material_name(tinygltf::Model& model, int material_index) {
    char buffer[50];

    itoa(material_index, buffer, 10);
    string matname = "MAT_" + string{&buffer[0]} + "_" + model.materials[material_index].name;
    return matname;
}

bool _convert_gltf_meshes(tinygltf::Model& model, const fs::path& output_folder) {
    for (auto i_mesh = 0; i_mesh < model.meshes.size(); i_mesh++) {
        auto& gltf_mesh = model.meshes[i_mesh];

        for (auto i_primitive = 0; i_primitive < gltf_mesh.primitives.size(); i_primitive++) {
            MeshCPU mesh_cpu;

            mesh_cpu.name      = _calculate_gltf_mesh_name(model, i_mesh, i_primitive);
            mesh_cpu.file_name = (output_folder / (mesh_cpu.name + mesh_extension)).string();

            auto& primitive = gltf_mesh.primitives[i_primitive];
            _extract_gltf_indices(primitive, model, mesh_cpu.indices);
            _extract_gltf_vertices(primitive, model, mesh_cpu.vertices);

            save_mesh(mesh_cpu);
        }
    }
    return true;
}

bool _convert_gltf_materials(tinygltf::Model& model, const fs::path& output_folder) {
    int material_number = 0;
    for (auto& glmat : model.materials) {
        string matname = _calculate_gltf_material_name(model, material_number++);

        auto& pbr = glmat.pbrMetallicRoughness;

        MaterialCPU material_cpu;

        array texture_indices = {pbr.baseColorTexture.index, pbr.metallicRoughnessTexture.index, glmat.normalTexture.index,
                                 glmat.emissiveTexture.index};
        array texture_files = {&material_cpu.color_asset_path, &material_cpu.orm_asset_path, &material_cpu.normal_asset_path,
                               &material_cpu.emissive_asset_path};

        array texture_names = {"baseColor", "metallicRoughness", "normals", "emissive"};
        for (u32 i = 0; i < 4; i++) {
            int texture_index = texture_indices[i];
            if (texture_index < 0)
                continue;
            auto image     = model.textures[texture_index];
            auto baseImage = model.images[image.source];

            if (baseImage.name == "")
                baseImage.name = texture_names[i];

            fs::path color_path = (output_folder / baseImage.name).string();
            color_path.replace_extension(texture_extension);
            vuk::Format format = vuk::Format::eR8G8B8A8Srgb;

            TextureCPU texture_cpu = {
                baseImage.name,
                color_path.string(),
                v2i{baseImage.width, baseImage.height},
                format,
                vector<u8>(&*baseImage.image.begin(), &*baseImage.image.begin() + baseImage.image.size())
            };
            save_texture(texture_cpu);
            *texture_files[i] = texture_cpu.file_name;
        }

        material_cpu.color_tint = Color((f32) pbr.baseColorFactor[0],
            (f32) pbr.baseColorFactor[1],
            (f32) pbr.baseColorFactor[2],
            (f32) pbr.baseColorFactor[3]);
        material_cpu.emissive_tint    = material_cpu.emissive_asset_path == "textures/white.sbtex" ? palette::black : palette::white;
        material_cpu.roughness_factor = pbr.roughnessFactor;
        material_cpu.metallic_factor  = pbr.metallicFactor;
        material_cpu.normal_factor    = material_cpu.normal_asset_path == "textures/white.sbtex" ? 0.0f : 0.5f;
        fs::path material_path        = output_folder / (matname + material_extension);
        material_cpu.file_name        = material_path.string();

        if (glmat.alphaMode.compare("BLEND") == 0) {
            // new_material.transparency = TransparencyMode_Transparent;
        } else {
            // new_material.transparency = TransparencyMode_Opaque;
        }

        save_material(material_cpu);
    }
    return true;
}

m44 _calculate_matrix(tinygltf::Node& node) {
    m44 matrix;

    // node has a matrix
    if (node.matrix.size() > 0) {
        for (int n = 0; n < 16; n++) {
            matrix[n] = node.matrix[n];
        }
    }
    // separate transform
    else {
        m44 translation = m44::identity();
        if (node.translation.size() > 0) {
            translation = math::translate(v3{(f32) node.translation[0], (f32) node.translation[1], (f32) node.translation[2]});
        }

        m44 rotation = m44::identity();
        if (node.rotation.size() > 0) {
            quat rot((f32) node.rotation[0], (f32) node.rotation[1], (f32) node.rotation[2], (f32) node.rotation[3]);
            rotation = math::quat2rotation44(rot);
        }

        m44 scale = m44::identity();
        if (node.scale.size() > 0) {
            scale = math::scale(v3{(f32) node.scale[0], (f32) node.scale[1], (f32) node.scale[2]});
        }
        matrix = (translation * rotation * scale); // * gltf_fixup;
    }

    return matrix;
};


}
