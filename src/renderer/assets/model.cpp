#include "model.hpp"

#include <tiny_gltf.h>

#include "extension/imgui_extra.hpp"
#include "extension/icons/font_awesome4.h"
#include "general/logger.hpp"
#include "general/matrix_math.hpp"
#include "game/game.hpp"
#include "renderer/renderer.hpp"
#include "renderer/renderable.hpp"
#include "renderer/render_scene.hpp"
#include "renderer/assets/mesh.hpp"
#include "renderer/assets/material.hpp"
#include "renderer/assets/mesh_asset.hpp"
#include "renderer/assets/texture_asset.hpp"

namespace spellbook {

vector<ModelCPU> ModelCPU::split() {
    auto traverse = [](ModelCPU& model, id_ptr<Node> node, auto&& traverse) -> void {
        for (id_ptr<Node> child : node->children) {
            model.nodes.push_back(child);
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
    ImGui::PathSelect("File", &model->file_path, "resources", FileType_Model);

    std::function<void(id_ptr<ModelCPU::Node>)> traverse;
    traverse = [&traverse, &model](id_ptr<ModelCPU::Node> node) {
        string id_str = fmt_("{}###{}", node->name, node.id);
        ImGui::SetNextItemOpen(true, ImGuiCond_Once);
        if (ImGui::TreeNode(id_str.c_str())) {
            ImGui::InputText("Name", &node->name);
            ImGui::PathSelect("mesh_asset_path", &node->mesh_asset_path, "resources", FileType_Mesh);
            ImGui::PathSelect("material_asset_path", &node->material_asset_path, "resources", FileType_Material);
            ImGui::DragMat4("Transform", &node->transform, 0.02f, "%.2f");
            for (auto child : node->children) {
                traverse(child);
            }
            if (ImGui::Button(ICON_FA_PLUS_CIRCLE, {-ImGui::GetStyle().FramePadding.x, 0})) {
                model->nodes.push_back(id_ptr<ModelCPU::Node>::emplace());
                            
                model->nodes.back()->parent = node;
                node->children.emplace_back(model->nodes.back());
            }
            ImGui::Separator();
            ImGui::TreePop();
        }
    };

    if (model->root_node.valid()) {
        traverse(model->root_node);
    } else {
        ImGui::Text("No Root Node");
    }
}


void save_model(const ModelCPU& model) {
    json j;
    j["root_node"] = make_shared<json_value>(to_jv(model.root_node));

    // TODO: Make this a one liner and integrate this, skeletons, and bones
    vector<json_value> json_nodes;
    for (id_ptr<ModelCPU::Node> node : model.nodes) {
        json_nodes.push_back(to_jv_full(node));
    }
    j["nodes"] = make_shared<json_value>(to_jv(json_nodes));

    vector<json_value> json_skeletons;
    for (id_ptr<SkeletonCPU> skeleton : model.skeletons) {
        json_skeletons.push_back(to_jv_full(skeleton));
    }
    j["skeletons"] = make_shared<json_value>(to_jv(json_skeletons));
    
    
    string ext = std::filesystem::path(model.file_path).extension().string();
    assert_else(ext == extension(FileType_Model));
    
    file_dump(j, to_resource_path(model.file_path).string());
}

fs::path _convert_to_relative(const fs::path& path) {
    return path.lexically_proximate(game.resource_folder);
}

ModelCPU load_model(const fs::path& input_path) {
    fs::path absolute_path = to_resource_path(input_path);
    check_else(fs::exists(absolute_path))
        return {};
    string ext = input_path.extension().string();
    check_else(ext == extension(FileType_Model))
        return {};
    
    ModelCPU model;
    json      j = parse_file(absolute_path.string());
    model.file_path = input_path.string();
    if (j.contains("root_node"))
        model.root_node = from_jv<id_ptr<ModelCPU::Node>>(*j["root_node"]);

    if (j.contains("nodes")) {
        for (const json_value& jv : j["nodes"]->get_list()) {
            id_ptr<ModelCPU::Node> node = from_jv_impl(jv, (id_ptr<ModelCPU::Node>*) 0);
            model.nodes.push_back(node);
        }
    }

    if (j.contains("skeletons")) {
        for (const json_value& jv : j["skeletons"]->get_list()) {
            id_ptr<SkeletonCPU> skeleton = from_jv_impl(jv, (id_ptr<SkeletonCPU>*) 0);
            model.skeletons.push_back(skeleton);
        }
    }
    
    return model;
}

ModelGPU instance_model(RenderScene& render_scene, const ModelCPU& model, bool frame) {
    ModelGPU model_gpu;

    umap<u64, u32> indices;
    for (id_ptr<SkeletonCPU> skeleton_cpu : model.skeletons) {
        model_gpu.skeletons.emplace_back(std::make_unique<SkeletonGPU>(game.renderer.upload_skeleton(*skeleton_cpu)));
        indices[skeleton_cpu.id] = indices.size();
    }
    
    for (id_ptr<ModelCPU::Node> node_ptr : model.nodes) {
        const ModelCPU::Node& node           = *node_ptr;
        auto new_renderable = render_scene.add_renderable(Renderable(
            node.mesh_asset_path,
            node.material_asset_path,
            node.transform,
            indices.contains(node.skeleton.id) ? &*model_gpu.skeletons[indices[node.skeleton.id]] : nullptr,
            frame
        ));
        model_gpu.renderables.push_back(new_renderable);
    }

    return model_gpu;
}

void deinstance_model(RenderScene& render_scene, const ModelGPU& model) {
    for (auto& r_slot : model.renderables) {
        render_scene.delete_renderable(r_slot);
    }
}

m44 ModelCPU::Node::calculate_transform() const {
    return !parent.valid() ? transform : parent->calculate_transform() * transform;
}


ModelCPU quick_model(const string& name, const string& mesh, const string& material) {
    ModelCPU model;
    model.root_node = id_ptr<ModelCPU::Node>::emplace(name, mesh, material, m44::identity());
    model.nodes.push_back(model.root_node);
    return model;
}

constexpr m44 gltf_fixup = m44(0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1);

void _unpack_gltf_buffer(tinygltf::Model& model, tinygltf::Accessor& accessor, vector<u8>& output_buffer);
void _extract_gltf_vertices(tinygltf::Primitive& primitive, tinygltf::Model& model, vector<Vertex>& vertices);
void _extract_gltf_indices(tinygltf::Primitive& primitive, tinygltf::Model& model, vector<u32>& indices);
string _calculate_gltf_mesh_name(tinygltf::Model& model, int mesh_index, int primitive_index);
string _calculate_gltf_material_name(tinygltf::Model& model, int material_index);
bool _convert_gltf_skeletons(tinygltf::Model& model, ModelCPU* model_cpu);
bool _convert_gltf_meshes(tinygltf::Model& model, const fs::path& output_folder);
bool _convert_gltf_materials(tinygltf::Model& model, const fs::path& output_folder);
m44 _calculate_matrix(tinygltf::Node& node);

ModelCPU convert_to_model(const fs::path& input_path, const fs::path& output_folder, const fs::path& output_name) {
    // TODO: gltf can't support null materials
    fs::create_directory(game.resource_folder);
    auto folder = game.resource_folder / output_folder;
    fs::create_directory(folder);

    const auto& ext = input_path.extension().string();
    assert_else(path_filter(FileType_ModelAsset)(input_path))
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

    ModelCPU model_cpu;
    _convert_gltf_skeletons(gltf_model, &model_cpu);
    _convert_gltf_meshes(gltf_model, output_folder);
    _convert_gltf_materials(gltf_model, output_folder);
    

    // calculate parent hierarchies
    for (u32 i = 0; i < gltf_model.nodes.size(); i++) {
        model_cpu.nodes.push_back(id_ptr<ModelCPU::Node>::emplace());
    }
    for (u32 i = 0; i < gltf_model.nodes.size(); i++) {
        auto& children = model_cpu.nodes[i]->children;
        for (u32 c : gltf_model.nodes[i].children) {
            children.push_back(model_cpu.nodes[c]);
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
                root_node->children.push_back(node);
            }
        }
        model_cpu.root_node = root_node;
        model_cpu.nodes.push_back(root_node);
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

            fs::path mesh_path     = output_folder / (mesh_name + extension(FileType_Mesh));
            fs::path material_path = output_folder / (material_name + extension(FileType_Material));

            model_node.name                = gltf_node.name;
            model_node.mesh_asset_path     = mesh_path.string();
            model_node.material_asset_path = material_path.string();
            model_node.transform           = _calculate_matrix(gltf_node);
            if (gltf_node.skin != -1)
                model_node.skeleton = model_cpu.skeletons[gltf_node.skin];
        }
        // Serving hierarchy
        else {
            model_node.name      = gltf_node.name;
            model_node.transform = _calculate_matrix(gltf_node);

            if (gltf_node.mesh >= 0)
                multimat_nodes.push_back(i);
        }
    }
    
    model_cpu.root_node->transform = gltf_fixup * model_cpu.root_node->transform;

    // iterate nodes with multiple materials, convert each submesh into a node
    for (u32 multimat_node_index : multimat_nodes) {
        auto& multimat_node       = gltf_model.nodes[multimat_node_index];

        if (multimat_node.mesh < 0)
            break;

        auto mesh = gltf_model.meshes[multimat_node.mesh];

        for (u32 i_primitive = 0; i_primitive < mesh.primitives.size(); i_primitive++) {
            auto                    primitive = mesh.primitives[i_primitive];
            id_ptr<ModelCPU::Node> model_node_ptr      = id_ptr<ModelCPU::Node>::emplace();
            model_cpu.nodes.push_back(model_node_ptr);

            char buffer[50];

            itoa(i_primitive, buffer, 10);

            int    material      = primitive.material;
            string material_name = _calculate_gltf_material_name(gltf_model, material);
            string mesh_name     = _calculate_gltf_mesh_name(gltf_model, multimat_node.mesh, i_primitive);

            fs::path material_path = output_folder / (material_name + extension(FileType_Material));
            fs::path mesh_path     = output_folder / (mesh_name + extension(FileType_Mesh));

            model_node_ptr->name      = model_cpu.nodes[multimat_node_index]->name + "_PRIM_" + &buffer[0];
            model_node_ptr->mesh_asset_path      = mesh_path.string();
            model_node_ptr->material_asset_path  = material_path.string();
            model_node_ptr->transform = m44::identity();
            model_node_ptr->parent    = model_cpu.nodes[multimat_node_index];
            model_cpu.nodes[multimat_node_index]->children.push_back(model_node_ptr);
            if (gltf_model.nodes[multimat_node_index].skin != -1)
                model_node_ptr->skeleton = model_cpu.skeletons[gltf_model.nodes[multimat_node_index].skin];
        }
    }

    fs::path scene_path = output_folder / output_name;
    scene_path.replace_extension(extension(FileType_Model));

    model_cpu.file_path = scene_path.string();

    return model_cpu;
}

bool _convert_gltf_skeletons(tinygltf::Model& model, ModelCPU* model_cpu) {
    umap<u32, id_ptr<Bone>> node_index_to_bone; 
    for (u32 i_skin = 0; i_skin < model.skins.size(); i_skin++) {
        model_cpu->skeletons.push_back(id_ptr<SkeletonCPU>::emplace());
        auto& skeleton = model_cpu->skeletons.back();
        for (u32 i_bone = 0; i_bone < model.skins[i_skin].joints.size(); i_bone++) {
            skeleton->bones.push_back(id_ptr<Bone>::emplace());
            u32 bone_node_index = model.skins[i_skin].joints[i_bone];
            node_index_to_bone[bone_node_index] = skeleton->bones.back();
            // node_index_to_bone[bone_node_index]->inverse_bind_matrix;

        }
        u32 ibm_index = model.skins[i_skin].inverseBindMatrices;
        tinygltf::Accessor& ibm_accessor = model.accessors[ibm_index];
        vector<u8> pos_data;
        _unpack_gltf_buffer(model, ibm_accessor, pos_data);
        
        for (int i = 0; i < ibm_accessor.count; i++) {
            check_else (ibm_accessor.type == TINYGLTF_TYPE_MAT4)
                continue;
            
            check_else (ibm_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
                continue;
            
            float* dtf = (float*) pos_data.data();

            model_cpu->skeletons[i_skin]->bones[i]->inverse_bind_matrix = {
                *(dtf + (i * 16) + 0), *(dtf + (i * 16) + 4), *(dtf + (i * 16) +  8), *(dtf + (i * 16) + 12),
                *(dtf + (i * 16) + 1), *(dtf + (i * 16) + 5), *(dtf + (i * 16) +  9), *(dtf + (i * 16) + 13),
                *(dtf + (i * 16) + 2), *(dtf + (i * 16) + 6), *(dtf + (i * 16) + 10), *(dtf + (i * 16) + 14),
                *(dtf + (i * 16) + 3), *(dtf + (i * 16) + 7), *(dtf + (i * 16) + 11), *(dtf + (i * 16) + 15)
            };
        }
    }
    for (auto& [node_index, bone_ptr] : node_index_to_bone) {
        for (u32 child_index : model.nodes[node_index].children) {
            if (node_index_to_bone.contains(child_index))
                node_index_to_bone[child_index]->parent = bone_ptr;
        }
        // m44 bone_matrix = _calculate_matrix(model.nodes[node_index]);

        bone_ptr->name = model.nodes[node_index].name;
        bone_ptr->start.scale.value = v3(1,1,1);
        
        if (model.nodes[node_index].translation.size() > 0) {
            auto translation = model.nodes[node_index].translation;
            bone_ptr->start.position.value = v3{(f32) translation[0], (f32) translation[1], (f32) translation[2]};
        }

        if (model.nodes[node_index].rotation.size() > 0) {
            bone_ptr->start.rotation.value = quat((f32) model.nodes[node_index].rotation[0], (f32) model.nodes[node_index].rotation[1], (f32) model.nodes[node_index].rotation[2], (f32) model.nodes[node_index].rotation[3]);
        }

        if (model.nodes[node_index].scale.size() > 0) {
            bone_ptr->start.scale.value = v3{(f32) model.nodes[node_index].scale[0], (f32) model.nodes[node_index].scale[1], (f32) model.nodes[node_index].scale[2]};
        }
        
        // // matrix = (translation * rotation * scale); // * gltf_fixup;
        // math::extract_tsr(bone_matrix,
        //     &bone_ptr->positions.back().position,
        //     &bone_ptr->scales.back().scale,
        //     &bone_ptr->rotations.back().orientation
        // );
    }
    return true;
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

    tinygltf::Accessor& tangent_accessor = model.accessors[primitive.attributes["TANGENT"]];
    vector<u8>          tangent_data;
    _unpack_gltf_buffer(model, tangent_accessor, tangent_data);
    for (int i = 0; i < vertices.size(); i++) {
        if (tangent_accessor.type == TINYGLTF_TYPE_VEC3) {
            if (tangent_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                float* dtf = (float*) tangent_data.data();

                if (tangent_data.size() > sizeof(float) * (i * 3 + Z)) {
                    vertices[i].tangent[X] = *(dtf + (i * 3) + X);
                    vertices[i].tangent[Y] = *(dtf + (i * 3) + Y);
                    vertices[i].tangent[Z] = *(dtf + (i * 3) + Z);
                }
            } else {
                assert_else(false);
            }
        } else if (tangent_accessor.type == TINYGLTF_TYPE_VEC4) {
            if (tangent_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                float* dtf = (float*) tangent_data.data();

                if (tangent_data.size() > sizeof(float) * (i * 4 + Z)) {
                    vertices[i].tangent[X] = *(dtf + (i * 4) + X);
                    vertices[i].tangent[Y] = *(dtf + (i * 4) + Y);
                    vertices[i].tangent[Z] = *(dtf + (i * 4) + Z);
                }
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
                log_error("Only FLOAT supported for UV coordinate in GLTF convert");
            }
        } else if (uv_accessor.type == TINYGLTF_TYPE_VEC3) {
            if (uv_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
                float* dtf = (float*) uv_data.data();

                // vec3f
                vertices[i].uv[0] = *(dtf + (i * 3) + 0);
                vertices[i].uv[1] = *(dtf + (i * 3) + 1);
            } else {
                log_error("Only FLOAT supported for UV coordinate in GLTF convert");
            }
        } else {
            log_error("Unsupported type for UV coordinate in GLTF convert");
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

    if (primitive.attributes["JOINTS_0"] != 0) {
        tinygltf::Accessor& joint_accessor = model.accessors[primitive.attributes["JOINTS_0"]];
        vector<u8>          joint_data;
        _unpack_gltf_buffer(model, joint_accessor, joint_data);
        for (int i = 0; i < vertices.size(); i++) {
            u8 components = 0;
            switch (joint_accessor.type) {
                case (TINYGLTF_TYPE_VEC3): {
                    components = 3;
                } break;
                case (TINYGLTF_TYPE_VEC4): {
                    components = 4;
                } break;
                default:
                    assert_else("NYI" && false);
            }
            for (u8 c = 0; c < components; c++) {
                switch (joint_accessor.componentType) {
                    case (TINYGLTF_COMPONENT_TYPE_INT): {
                        auto dtf = (s32*) joint_data.data();
                        vertices[i].bone_ids[c] = *(dtf + i * components + c);
                    } break;
                    case (TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT): {
                        auto dtf = (u32*) joint_data.data();
                        vertices[i].bone_ids[c] = *(dtf + i * components + c);
                    } break;
                    case (TINYGLTF_COMPONENT_TYPE_SHORT): {
                        auto dtf = (s16*) joint_data.data();
                        vertices[i].bone_ids[c] = *(dtf + i * components + c);
                    } break;
                    case (TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT): {
                        auto dtf = (u16*) joint_data.data();
                        vertices[i].bone_ids[c] = *(dtf + i * components + c);
                    } break;
                    case (TINYGLTF_COMPONENT_TYPE_BYTE): {
                        auto dtf = (s8*) joint_data.data();
                        vertices[i].bone_ids[c] = *(dtf + i * components + c);
                    } break;
                    case (TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE): {
                        auto dtf = (u8*) joint_data.data();
                        vertices[i].bone_ids[c] = *(dtf + i * components + c);
                    } break;
                    default:
                        assert_else("NYI" && false);
                }
            }
        }
    }

    if (primitive.attributes["WEIGHTS_0"] != 0) {
        tinygltf::Accessor& weight_accessor = model.accessors[primitive.attributes["WEIGHTS_0"]];
        vector<u8>          weight_data;
        _unpack_gltf_buffer(model, weight_accessor, weight_data);
        for (int i = 0; i < vertices.size(); i++) {
            u8 components = 0;
            switch (weight_accessor.type) {
                case (TINYGLTF_TYPE_VEC3): {
                    components = 3;
                } break;
                case (TINYGLTF_TYPE_VEC4): {
                    components = 4;
                } break;
                default:
                    assert_else("NYI" && false);
            }
            for (u8 c = 0; c < components; c++) {
                switch (weight_accessor.componentType) {
                    case (TINYGLTF_COMPONENT_TYPE_FLOAT): {
                        auto dtf = (f32*) weight_data.data();
                        vertices[i].bone_weights[c] = *(dtf + i * components + c);
                    } break;
                    case (TINYGLTF_COMPONENT_TYPE_DOUBLE): {
                        auto dtf = (f64*) weight_data.data();
                        vertices[i].bone_weights[c] = *(dtf + i * components + c);
                    } break;
                    default:
                        assert_else("NYI" && false);
                }
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
                log_error("Only SHORT/USHORT supported for index in GLTF convert");
        }

        indices.push_back(index);
    }

    for (u32 i = 0; i < indices.size() / 3; i++) {
        // flip the triangle
        std::swap(indices[i * 3 + 1], indices[i * 3 + 2]);
    }
}

string _calculate_gltf_mesh_name(tinygltf::Model& model, int mesh_index, int primitive_index) {
    char prim_buffer[50];
    itoa(primitive_index, prim_buffer, 10);

    string meshname = model.meshes[mesh_index].name;

    bool multiprim = model.meshes[mesh_index].primitives.size() > 1;
    if (multiprim) {
        meshname += "_sub_" + string{&prim_buffer[0]};
    }

    return meshname;
}

string _calculate_gltf_material_name(tinygltf::Model& model, int material_index) {
    string matname = model.materials[material_index].name;
    return matname;
}

bool _convert_gltf_meshes(tinygltf::Model& model, const fs::path& output_folder) {
    for (u32 i_mesh = 0; i_mesh < model.meshes.size(); i_mesh++) {
        auto& gltf_mesh = model.meshes[i_mesh];

        for (auto i_primitive = 0; i_primitive < gltf_mesh.primitives.size(); i_primitive++) {
            MeshCPU mesh_cpu;

            string name = _calculate_gltf_mesh_name(model, i_mesh, i_primitive);
            mesh_cpu.file_path = (output_folder / (name + extension(FileType_Mesh))).string();

            auto& primitive = gltf_mesh.primitives[i_primitive];
            _extract_gltf_indices(primitive, model, mesh_cpu.indices);
            _extract_gltf_vertices(primitive, model, mesh_cpu.vertices);

            if (math::length(mesh_cpu.vertices.back().tangent) < 0.1f) {
                mesh_cpu.fix_tangents();
            }
            
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
            color_path.replace_extension(extension(FileType_Texture));
            vuk::Format format = vuk::Format::eR8G8B8A8Srgb;

            TextureCPU texture_cpu = {
                color_path.string(),
                v2i{baseImage.width, baseImage.height},
                format,
                vector<u8>(&*baseImage.image.begin(), &*baseImage.image.begin() + baseImage.image.size())
            };
            save_texture(texture_cpu);
            *texture_files[i] = texture_cpu.file_path;
        }

        material_cpu.color_tint = Color((f32) pbr.baseColorFactor[0],
            (f32) pbr.baseColorFactor[1],
            (f32) pbr.baseColorFactor[2],
            (f32) pbr.baseColorFactor[3]);
        material_cpu.emissive_tint    = material_cpu.emissive_asset_path == "textures/white.sbtex" ? palette::black : palette::white;
        material_cpu.roughness_factor = pbr.roughnessFactor;
        material_cpu.metallic_factor  = pbr.metallicFactor;
        material_cpu.normal_factor    = material_cpu.normal_asset_path == "textures/white.sbtex" ? 0.0f : 0.5f;
        fs::path material_path        = output_folder / (matname + extension(FileType_Material));
        material_cpu.file_path        = material_path.string();

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
    m44 matrix = {};

    // node has a matrix
    if (node.matrix.size() > 0) {
        for (int n = 0; n < 16; n++) {
            matrix[n] = f32(node.matrix[n]);
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
            rotation = math::rotation(rot);
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
