#include "prefab.hpp"

#include <filesystem>

#include "tiny_gltf.h"

#include "game.hpp"
#include "matrix_math.hpp"
#include "mesh_asset.hpp"
#include "texture_asset.hpp"

#include "renderer/renderer.hpp"
#include "renderer/renderable.hpp"
#include "renderer/render_scene.hpp"
#include "renderer/assets/mesh.hpp"
#include "renderer/assets/material.hpp"

namespace fs = std::filesystem;

namespace spellbook {

vector<PrefabCPU> PrefabCPU::split() {
    auto traverse = [](PrefabCPU& prefab, id_ptr<PrefabCPU::Node> node, auto&& traverse) -> void {
        for (id_ptr<Node> child : node->children) {
            prefab.nodes.insert_back(child);
            traverse(prefab, child, traverse);
        }
    };

    vector<PrefabCPU> prefabs;
    prefabs.resize(root_node->children.size());
    u32 i = 0;
    for (id_ptr<Node> child : root_node->children) {
        child->parent = id_ptr<Node>::null();
        traverse(prefabs[i++], child, traverse);
    }

    return prefabs;
}

void save_prefab(const PrefabCPU& prefab) {
    json j;
    j["root_node"] = make_shared<json_value>(prefab.root_node);

    vector<json_value> json_nodes;
    for (id_ptr<PrefabCPU::Node> node : prefab.nodes) {
        json json_node;
        json_node["node"] = make_shared<json_value>(*node);
        json_node["id"]   = make_shared<json_value>(node);
        json_nodes.insert_back((json_value) json_node);
    }
    j["nodes"] = make_shared<json_value>(json_nodes);
    file_dump(j, prefab.file_name);
}

PrefabCPU load_prefab(const string_view file_name) {
    PrefabCPU prefab;
    json      j = parse_file(file_name);
    if (j.contains("root_node"))
        prefab.root_node = id_ptr<PrefabCPU::Node>(*j["root_node"]);

    if (j.contains("nodes")) {
        for (const json_value& jv : j["nodes"]->get_list()) {
            auto json_node = json(jv);
            json_node["node"];
            json_node["id"];
        }
    }
    return prefab;
}

PrefabGPU instance_prefab(RenderScene& render_scene, const PrefabCPU& prefab) {
    PrefabGPU prefab_gpu;

    for (id_ptr<PrefabCPU::Node> node_ptr : prefab.nodes) {
        const PrefabCPU::Node& node = *node_ptr;
        Renderable renderable = Renderable(game.renderer.find_mesh(node.name), game.renderer.find_material(node.material), node.transform);
        auto new_renderable = render_scene.add_renderable(renderable);
        prefab_gpu.renderables.insert_back(new_renderable);
    }

    return prefab_gpu;
}

constexpr m44 gltf_fixup = m44(0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1);

fs::path _convert_to_relative(const fs::path& path) {
    return path.lexically_proximate(game.resource_folder);
}

void _unpack_gltf_buffer(tinygltf::Model& model, tinygltf::Accessor& accessor, vector<u8>& output_buffer) {
    int                   buffer_id     = accessor.bufferView;
    size_t                element_bsize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
    tinygltf::BufferView& buffer_view   = model.bufferViews[buffer_id];
    tinygltf::Buffer&     buffer_data   = (model.buffers[buffer_view.buffer]);
    u8*                   dataptr       = buffer_data.data.data() + accessor.byteOffset + buffer_view.byteOffset;
    int                   components    = tinygltf::GetNumComponentsInType(accessor.type);
    element_bsize *= components;
    size_t stride = buffer_view.byteStride;
    if (stride == 0) {
        stride = element_bsize;
    }

    output_buffer.resize(accessor.count * element_bsize);

    for (int i = 0; i < accessor.count; i++) {
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

    for (int i = 0; i < indices.size() / 3; i++) {
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
            mesh_cpu.file_name = (output_folder / (mesh_cpu.name + ".sbmsh")).string();

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
        array texture_files = {&material_cpu.base_color_texture, &material_cpu.orm_texture, &material_cpu.normal_texture,
                               &material_cpu.emissive_texture};

        for (int i = 0; i < 4; i++) {
            int texture_index = texture_indices[i];
            if (texture_index < 0)
                continue;
            auto image     = model.textures[texture_index];
            auto baseImage = model.images[image.source];

            constexpr string texture_names[] = {"baseColor", "metallicRoughness", "normals", "emissive"};
            if (baseImage.name == "")
                baseImage.name = texture_names[i];

            fs::path base_color_path = _convert_to_relative(output_folder / baseImage.name);
            base_color_path.replace_extension(".tx");
            vuk::Format format = vuk::Format::eR8G8B8A8Srgb;

            TextureCPU texture_cpu = {
                baseImage.name,
                base_color_path.string(),
                v2i{baseImage.width, baseImage.height},
                format,
                vector<u8>(&*baseImage.image.begin(), &*baseImage.image.end())
            };
            save_texture(texture_cpu);
            *texture_files[i] = texture_cpu.file_name;
        }

        material_cpu.base_color_tint = Color((f32) pbr.baseColorFactor[0],
            (f32) pbr.baseColorFactor[1],
            (f32) pbr.baseColorFactor[2],
            (f32) pbr.baseColorFactor[3]);
        material_cpu.emissive_tint    = material_cpu.emissive_texture == "textures/white.sbtex" ? palette::black : palette::white;
        material_cpu.roughness_factor = pbr.roughnessFactor;
        material_cpu.metallic_factor  = pbr.metallicFactor;
        material_cpu.normal_factor    = material_cpu.normal_texture == "textures/white.sbtex" ? 0.0f : 0.5f;
        fs::path material_path        = output_folder / (matname + ".sbmat");
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

PrefabCPU convert_to_prefab(const string& file_name, const string& output_folder, const string& output_name) {
    // TODO: gltf can't support null materials
    fs::create_directory(game.resource_folder);

    const fs::path file_path = fs::path(file_name);

    const auto& ext = file_path.extension().string();
    assert_else(ext == ".gltf" || ext == ".glb")
        return {};
    tinygltf::Model    model;
    tinygltf::TinyGLTF loader;
    string             err, warn;

    bool ret = ext == ".gltf"
                   ? loader.LoadASCIIFromFile(&model, &err, &warn, file_name)
                   : loader.LoadBinaryFromFile(&model, &err, &warn, file_name);

    if (!warn.empty())
        console_error(fmt_("Conversion warning while loading \"{}\": {}", file_name, warn), "asset.import", ErrorType_Warning);
    if (!err.empty())
        console_error(fmt_("Conversion error while loading \"{}\": {}", file_name, err), "asset.import", ErrorType_Severe);
    assert_else(!ret)
        return {};

    auto folder = game.resource_folder / fs::path(file_path.stem().string() + "_GLTF");
    fs::create_directory(folder);

    PrefabCPU prefab;

    // calculate parent hierarchies
    for (u32 i = 0; i < model.nodes.size(); i++) {
        prefab.nodes.insert_back(id_ptr<PrefabCPU::Node>());
    }
    for (u32 i = 0; i < model.nodes.size(); i++) {
        auto& children = prefab.nodes[i]->children;
        for (u32 c : model.nodes[i].children) {
            children.insert(prefab.nodes[c]);
            prefab.nodes[c]->parent = prefab.nodes[i];
        }
    }

    vector<u32> multimat_nodes;
    for (int i = 0; i < model.nodes.size(); i++) {
        auto& node = model.nodes[i];

        if (node.mesh >= 0 && model.meshes[node.mesh].primitives.size() == 1) {
            auto mesh = model.meshes[node.mesh];

            auto primitive = mesh.primitives[0];
            int  material  = primitive.material;
            assert_else(material >= 0 && "Need material on node, default mat NYI");
            string mesh_name     = _calculate_gltf_mesh_name(model, node.mesh, 0);
            string material_name = _calculate_gltf_material_name(model, material);

            fs::path meshpath     = _convert_to_relative(fs::path(output_folder) / (mesh_name + ".sbmsh"));
            fs::path materialpath = _convert_to_relative(fs::path(output_folder) / (material_name + ".sbmat"));

            prefab.nodes[i]->name      = model.nodes[i].name;
            prefab.nodes[i]->mesh      = meshpath.string();
            prefab.nodes[i]->material  = materialpath.string();
            prefab.nodes[i]->transform = _calculate_matrix(node);
        }
        // Serving hierarchy
        else {
            prefab.nodes[i]->name      = node.name;
            prefab.nodes[i]->transform = _calculate_matrix(node);

            if (node.mesh >= 0)
                multimat_nodes.insert_back(i);
        }
    }

    for (int i = 0; i < prefab.nodes.size(); i++) {
        if (!prefab.nodes[i]->parent.valid())
            prefab.nodes[i]->transform = gltf_fixup * prefab.nodes[i]->transform;
    }

    int nodeindex = prefab.nodes.size();
    // iterate nodes with multiple materials, convert each submesh into a node
    for (int i = 0; i < multimat_nodes.size(); i++) {
        auto  multimat_node_index = multimat_nodes[i];
        auto& multimat_node       = model.nodes[multimat_node_index];

        if (multimat_node.mesh < 0)
            break;

        auto mesh = model.meshes[multimat_node.mesh];

        for (int i_primitive = 0; i_primitive < mesh.primitives.size(); i_primitive++) {
            auto primitive = mesh.primitives[i_primitive];
            int  new_node  = nodeindex++;
            prefab.nodes.insert_back(id_ptr<PrefabCPU::Node>());
            id_ptr<PrefabCPU::Node> node = prefab.nodes.last();

            char buffer[50];

            itoa(i_primitive, buffer, 10);

            int    material      = primitive.material;
            string material_name = _calculate_gltf_material_name(model, material);
            string mesh_name     = _calculate_gltf_mesh_name(model, multimat_node.mesh, i_primitive);

            fs::path material_path = _convert_to_relative(fs::path(output_folder) / (material_name + ".sbmat"));
            fs::path mesh_path     = _convert_to_relative(fs::path(output_folder) / (mesh_name + ".sbmsh"));

            node->name      = prefab.nodes[multimat_node_index]->name + "_PRIM_" + &buffer[0];
            node->mesh      = mesh_path.string();
            node->material  = material_path.string();
            node->transform = m44::identity();
            node->parent    = prefab.nodes[multimat_node_index];
            prefab.nodes[multimat_node_index]->children.insert(node);
        }
    }

    fs::path scene_path = output_folder / fs::path(output_name);
    scene_path.replace_extension(".sbpfb");

    prefab.file_name = scene_path.string();

    return prefab;
}


}
