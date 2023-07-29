#include "model.hpp"

#include <tiny_gltf.h>
#include <tracy/Tracy.hpp>
#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "extension/fmt.hpp"
#include "extension/fmt_geometry.hpp"
#include "extension/imgui_extra.hpp"
#include "extension/icons/font_awesome4.h"
#include "general/logger.hpp"
#include "general/math/matrix_math.hpp"
#include "game/game.hpp"
#include "game/game_file.hpp"
#include "editor/console.hpp"
#include "renderer/renderer.hpp"
#include "renderer/renderable.hpp"
#include "renderer/render_scene.hpp"
#include "renderer/assets/mesh.hpp"
#include "renderer/assets/material.hpp"
#include "renderer/assets/mesh_asset.hpp"
#include "renderer/assets/texture_asset.hpp"

namespace spellbook {

ModelCPU& ModelCPU::operator = (const ModelCPU& oth) {
    file_path = oth.file_path;
    dependencies = oth.dependencies;
    umap<uint64, uint64> old_to_new;
    for (id_ptr<Node> old_node : oth.nodes) {
        id_ptr<Node> new_node = id_ptr<Node>::emplace(
            old_node->name,
            old_node->mesh_asset_path,
            old_node->material_asset_path,
            old_node->transform,
            old_node->parent,
            old_node->children
        );
        old_to_new[old_node.id] = new_node.id;
        nodes.push_back(new_node);
    }

    // remap ids
    root_node = old_to_new[oth.root_node.id];
    for (id_ptr<Node> node : nodes) {
        if (node->parent.id != 0)
            node->parent.id = old_to_new[node->parent.id];
        for (id_ptr<Node>& child : node->children)
            child.id = old_to_new[child.id];
    }
    if (!root_node.valid())
        return *this;
    
    root_node->cache_transform();
    
    if (oth.skeleton != nullptr)
        skeleton = std::make_unique<SkeletonCPU>(instance_prefab(*oth.skeleton->prefab));
    return *this;
}

ModelCPU::ModelCPU(const ModelCPU& oth) {
    *this = oth;
}

vector<ModelCPU> ModelCPU::split() {
    // This should generate new IDs to not destroy the old model
    auto traverse = [](ModelCPU& model, id_ptr<Node> node, auto&& traverse) -> void {
        model.nodes.push_back(node);
        for (id_ptr<Node> child : node->children) {
            traverse(model, child, traverse);
        }
    };

    vector<ModelCPU> models;
    models.resize(root_node->children.size());
    uint32 i = 0;
    for (id_ptr<Node> child : root_node->children) {
        child->parent = id_ptr<Node>::null();
        child->transform = m44::identity();
        child->cache_transform();
        // TODO
        models[i].file_path;
        traverse(models[i++], child, traverse);
    }

    return models;
}

bool inspect(ModelCPU* model, RenderScene* render_scene) {
    ImGui::PathSelect("File", &model->file_path, FileType_Model, false);

    bool changed = false;

    changed |= inspect_dependencies(model->dependencies, model->file_path);
    
    std::function<void(id_ptr<ModelCPU::Node>)> traverse;
    traverse = [&traverse, &model, &changed](id_ptr<ModelCPU::Node> node) {
        string id_str = fmt_("{}###{}", node->name, node.id);
        if (ImGui::TreeNode(id_str.c_str())) {
            ImGui::InputText("Name", &node->name);
            ImGui::Text("Index: %d", model->nodes.find(node));
            changed |= ImGui::PathSelect("mesh_asset_path", &node->mesh_asset_path, FileType_Mesh);
            changed |= ImGui::PathSelect("material_asset_path", &node->material_asset_path, FileType_Material);
            if (ImGui::TreeNode("Transform")) {
                if (ImGui::DragMat4("##Transform", &node->transform, 0.02f, "%.2f")) {
                    changed = true;
                    node->cache_transform();
                }
                ImGui::TreePop();
            }
            ImGui::Text("Children");
            ImGui::Indent();
            for (auto child : node->children) {
                traverse(child);
            }
            if (ImGui::Button("  " ICON_FA_PLUS_CIRCLE "  Add Child  ")) {
                model->nodes.push_back(id_ptr<ModelCPU::Node>::emplace());
                            
                model->nodes.back()->parent = node;
                node->children.emplace_back(model->nodes.back());
                changed = true;
            }
            ImGui::Separator();
            ImGui::Unindent();
            ImGui::TreePop();
        }
    };

    if (model->root_node.valid()) {
        traverse(model->root_node);
    } else {
        ImGui::Text("No Root Node");
    }

    if (model->skeleton != nullptr) {
        if (ImGui::TreeNode("Skeleton")) {
            inspect(&*model->skeleton);
            ImGui::TreePop();
        }
    }
    
    return changed;
}

ModelGPU instance_model(RenderScene& render_scene, const ModelCPU& model, bool frame) {
    ModelGPU model_gpu;

    if (model.skeleton)
        model_gpu.skeleton = std::make_unique<SkeletonGPU>(upload_skeleton(*model.skeleton));
    
    for (id_ptr<ModelCPU::Node> node_ptr : model.nodes) {
        ModelCPU::Node& node           = *node_ptr;
        if (!node.mesh_asset_path.is_file() || !node.material_asset_path.is_file())
            continue;

        uint64 mesh_id = hash_path(node.mesh_asset_path);
        uint64 material_id = hash_path(node.material_asset_path);
        game.renderer.file_path_cache[mesh_id] = node.mesh_asset_path;
        game.renderer.file_path_cache[material_id] = node.material_asset_path;

        auto new_renderable = render_scene.add_renderable(Renderable(
            mesh_id,
            material_id,
            (m44GPU) node.transform,
            model_gpu.skeleton ? &*model_gpu.skeleton : nullptr,
            frame
        ));
        model_gpu.renderables[&node] = new_renderable;
    }

    return model_gpu;
}

vector<StaticRenderable*> instance_static_model(RenderScene& render_scene, const ModelCPU& model) {
    vector<StaticRenderable*> renderables;
    for (id_ptr<ModelCPU::Node> node_ptr : model.nodes) {
        ModelCPU::Node& node           = *node_ptr;
        if (!node.mesh_asset_path.is_file() || !node.material_asset_path.is_file())
            continue;

        uint64 mesh_id = hash_path(node.mesh_asset_path);
        uint64 material_id = hash_path(node.material_asset_path);
        game.renderer.file_path_cache[mesh_id] = node.mesh_asset_path;
        game.renderer.file_path_cache[material_id] = node.material_asset_path;

        game.renderer.get_mesh_or_upload(mesh_id);
        game.renderer.get_material_or_upload(material_id);

        renderables.push_back(&*render_scene.static_renderables.emplace(StaticRenderable{
            mesh_id,
            material_id
        }));
    }

    return renderables;
}

template<>
bool save_asset(const ModelCPU& model) {
    json j;
    j["dependencies"] = make_shared<json_value>(to_jv(model.dependencies));
    j["root_node"] = make_shared<json_value>(to_jv(model.root_node));

    vector<json_value> json_nodes;
    for (id_ptr<ModelCPU::Node> node : model.nodes) {
        json_nodes.push_back(to_jv_full(node));
    }
    j["nodes"] = make_shared<json_value>(to_jv(json_nodes));
    if (model.skeleton)
        if (model.skeleton->prefab) {
            save_asset<SkeletonPrefab>(*model.skeleton->prefab);
            j["skeleton"] = make_shared<json_value>(to_jv(model.skeleton->prefab->file_path));
        }
    
    string ext = model.file_path.rel_path().extension().string();
    assert_else(ext == extension(FileType_Model))
        return false;
    
    file_dump(j, model.file_path.abs_string());
    return true;
}

fs::path _convert_to_relative(const fs::path& path) {
    return path.lexically_proximate(game.resource_folder);
}

template<>
ModelCPU& load_asset(const FilePath& input_path, bool assert_exists, bool clear_cache) {
    fs::path absolute_path = input_path.abs_path();
    if (clear_cache && cpu_asset_cache<ModelCPU>().contains(input_path))
        cpu_asset_cache<ModelCPU>().erase(input_path);
    if (cpu_asset_cache<ModelCPU>().contains(input_path))
        return *cpu_asset_cache<ModelCPU>()[input_path];

    ModelCPU& model = *cpu_asset_cache<ModelCPU>().emplace(input_path, std::make_unique<ModelCPU>()).first->second;
    
    string ext = absolute_path.extension().string();
    bool exists = fs::exists(absolute_path);
    bool corrext = ext == extension(from_typeinfo(typeid(ModelCPU)));
    if (assert_exists) {
        assert_else(exists && corrext)
            return model;
    } else {
        check_else(exists && corrext)
            return model;
    }

    json& j = game.asset_system.load_json(input_path);
    model.file_path = input_path;
    model.dependencies = game.asset_system.load_dependencies(j);
    
    if (j.contains("nodes")) {
        for (const json_value& jv : j["nodes"]->get_list()) {
            id_ptr<ModelCPU::Node> node = from_jv_impl(jv, (id_ptr<ModelCPU::Node>*) 0);
            model.nodes.push_back(node);
        }
    }
    
    if (j.contains("root_node"))
        model.root_node = from_jv<id_ptr<ModelCPU::Node>>(*j["root_node"]);

    if (model.root_node.id == 0)
        for (auto node : model.nodes)
            if (!node->parent.valid())
                model.root_node = node;

    if (j.contains("skeleton")) {
        model.skeleton = std::make_unique<SkeletonCPU>(instance_prefab(
            load_asset<SkeletonPrefab>(from_jv<FilePath>(*j["skeleton"]), true)
        ));
    }
    model.root_node->cache_transform();
    
    return model;
}

void deinstance_model(RenderScene& render_scene, const ModelGPU& model) {
    for (auto& [_, r] : model.renderables) {
        render_scene.delete_renderable(r);
    }
}

void deinstance_static_model(RenderScene& render_scene, const vector<StaticRenderable*>& model) {
    for (auto r : model) {
        render_scene.delete_renderable(r);
    }
}

void ModelCPU::Node::cache_transform() {
    cached_transform = !parent.valid() ? transform : parent->cached_transform * transform;
    for (auto child : children)
        child->cache_transform();
}

constexpr m44 gltf_fixup = m44(0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1);

void _unpack_gltf_buffer(tinygltf::Model& model, tinygltf::Accessor& accessor, vector<uint8>& output_buffer);
void _extract_gltf_vertices(tinygltf::Primitive& primitive, tinygltf::Model& model, vector<Vertex>& vertices);
void _extract_gltf_indices(tinygltf::Primitive& primitive, tinygltf::Model& model, vector<uint32>& indices);
string _calculate_gltf_mesh_name(tinygltf::Model& model, int mesh_index, int primitive_index);
string _calculate_gltf_material_name(tinygltf::Model& model, int material_index);
bool _convert_gltf_skeletons(tinygltf::Model& model, ModelCPU* model_cpu, bool replace_existing_pose);
bool _convert_gltf_meshes(tinygltf::Model& model, const FilePath& output_folder);
bool _convert_gltf_materials(tinygltf::Model& model, const FilePath& output_folder);
m44 _calculate_matrix(tinygltf::Node& node);

ModelCPU convert_to_model(const FilePath& input_path, const FilePath& output_folder, const string& output_name, bool y_up, bool replace_existing_poses) {
    ZoneScoped;
    // TODO: gltf can't support null materials
    // TODO: create dir

    fs::path output_folder_path = output_folder.abs_path();

    const auto& ext = input_path.rel_path().extension().string();
    assert_else(path_filter(FileType_ModelAsset)(input_path.abs_path()))
        return {};
    tinygltf::Model    gltf_model;
    tinygltf::TinyGLTF loader;
    string             err, warn;

    bool ret = ext == ".gltf"
       ? loader.LoadASCIIFromFile(&gltf_model, &err, &warn, input_path.abs_string())
       : loader.LoadBinaryFromFile(&gltf_model, &err, &warn, input_path.abs_string());

    if (!warn.empty())
        console_error(fmt_("Conversion warning while loading \"{}\": {}", input_path.abs_string(), warn), "asset.import", ErrorType_Warning);
    if (!err.empty())
        console_error(fmt_("Conversion error while loading \"{}\": {}", input_path.abs_string(), err), "asset.import", ErrorType_Severe);
    assert_else(ret)
        return {};

    ModelCPU model_cpu;
    fs::path model_fs_path = output_folder_path / output_name;
    model_fs_path.replace_extension(extension(FileType_Model));
    model_cpu.file_path = FilePath(model_fs_path);
    
    _convert_gltf_skeletons(gltf_model, &model_cpu, replace_existing_poses);
    _convert_gltf_meshes(gltf_model, output_folder);
    _convert_gltf_materials(gltf_model, output_folder);
    

    // calculate parent hierarchies
    for (uint32 i = 0; i < gltf_model.nodes.size(); i++) {
        model_cpu.nodes.push_back(id_ptr<ModelCPU::Node>::emplace());
    }
    for (uint32 i = 0; i < gltf_model.nodes.size(); i++) {
        auto& children = model_cpu.nodes[i]->children;
        for (uint32 c : gltf_model.nodes[i].children) {
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

    vector<uint32> multimat_nodes;
    for (uint32 i = 0; i < gltf_model.nodes.size(); i++) {
        auto& gltf_node = gltf_model.nodes[i];
        auto& model_node = *model_cpu.nodes[i];

        if (gltf_node.mesh >= 0 && gltf_model.meshes[gltf_node.mesh].primitives.size() == 1) {
            auto mesh = gltf_model.meshes[gltf_node.mesh];

            auto primitive = mesh.primitives[0];
            int  material  = primitive.material;
            assert_else(material >= 0 && "Need material on node, default mat NYI");
            string mesh_name     = _calculate_gltf_mesh_name(gltf_model, gltf_node.mesh, 0);
            string material_name = _calculate_gltf_material_name(gltf_model, material);

            fs::path mesh_path     = output_folder_path / (mesh_name + extension(FileType_Mesh));
            fs::path material_path = output_folder_path / (material_name + extension(FileType_Material));

            model_node.name                = gltf_node.name;
            model_node.mesh_asset_path     = FilePath(mesh_path);
            model_node.material_asset_path = FilePath(material_path);
            model_node.transform           = _calculate_matrix(gltf_node);
            if (gltf_node.skin != -1) {
                assert_else(gltf_node.skin == 0);
            }
        }
        // Serving hierarchy
        else {
            model_node.name      = gltf_node.name;
            model_node.transform = _calculate_matrix(gltf_node);

            if (gltf_node.mesh >= 0)
                multimat_nodes.push_back(i);
        }
    }

    if (y_up)
        model_cpu.root_node->transform = gltf_fixup * model_cpu.root_node->transform;

    // iterate nodes with multiple materials, convert each submesh into a node
    for (uint32 multimat_node_index : multimat_nodes) {
        auto& multimat_node       = gltf_model.nodes[multimat_node_index];

        if (multimat_node.mesh < 0)
            break;

        auto mesh = gltf_model.meshes[multimat_node.mesh];

        for (uint32 i_primitive = 0; i_primitive < mesh.primitives.size(); i_primitive++) {
            auto                    primitive = mesh.primitives[i_primitive];
            id_ptr<ModelCPU::Node> model_node_ptr      = id_ptr<ModelCPU::Node>::emplace();
            model_cpu.nodes.push_back(model_node_ptr);

            char buffer[50];

            itoa(i_primitive, buffer, 10);

            int    material      = primitive.material;
            string material_name = _calculate_gltf_material_name(gltf_model, material);
            string mesh_name     = _calculate_gltf_mesh_name(gltf_model, multimat_node.mesh, i_primitive);

            fs::path material_path = output_folder_path / (material_name + extension(FileType_Material));
            fs::path mesh_path     = output_folder_path / (mesh_name + extension(FileType_Mesh));

            model_node_ptr->name      = model_cpu.nodes[multimat_node_index]->name + "_PRIM_" + &buffer[0];
            model_node_ptr->mesh_asset_path      = FilePath(mesh_path);
            model_node_ptr->material_asset_path  = FilePath(material_path);
            model_node_ptr->transform = m44::identity();
            model_node_ptr->parent    = model_cpu.nodes[multimat_node_index];
            model_cpu.nodes[multimat_node_index]->children.push_back(model_node_ptr);
        }
    }

    {
        ZoneScopedN("Remove unused nodes");
        vector<uint8> used;
        used.resize(model_cpu.nodes.size());
        auto traverse = [&model_cpu, &used](id_ptr<ModelCPU::Node>& node, bool& uses, auto&& traverse) -> void {
            bool this_uses = false;
            bool children_uses = false;
            for (id_ptr<ModelCPU::Node>& child : node->children) {
                traverse(child, children_uses, traverse);
            }
            
            if (node->material_asset_path.is_file() && node->mesh_asset_path.is_file())
                this_uses = true;
            if (children_uses)
                this_uses = true;
    
            if (this_uses) {
                uses = true;
                used[model_cpu.nodes.find(node)] = true;
            }
        };
    
        bool any_used = false;
        uint32 root_index = model_cpu.nodes.find(model_cpu.root_node);
        traverse(model_cpu.nodes[root_index], any_used, traverse);
    
        uint32 removed = 0;
        for (uint32 i = 0; i < used.size(); i++) {
            if (!used[i]) {
                auto& this_node = model_cpu.nodes[i - removed];
                check_else (this_node->parent.valid())
                    continue;
                this_node->parent->children.remove_value(this_node);
                model_cpu.nodes.remove_index(i - removed, false);
                removed++;
            }
        }
    }

    model_cpu.root_node->cache_transform();

    return model_cpu;
}

bool _convert_gltf_skeletons(tinygltf::Model& model, ModelCPU* model_cpu, bool replace_existing_pose) {
    ZoneScoped;
    umap<uint32, id_ptr<BonePrefab>> node_index_to_bone;
    assert_else(model.skins.size() <= 1);

    if (model.skins.empty())
        return true;
    
    fs::path skeleton_fs_path = model_cpu->file_path.abs_path();
    skeleton_fs_path.replace_extension(extension(FileType_Skeleton));
    FilePath skeleton_path = FilePath(skeleton_fs_path);
    SkeletonPrefab& skeleton = load_asset<SkeletonPrefab>(skeleton_path, false);
    skeleton.file_path = skeleton_path;
    
    for (uint32 i_bone = 0; i_bone < model.skins[0].joints.size(); i_bone++) {
        if (skeleton.bones.size() <= i_bone)
            skeleton.bones.push_back(id_ptr<BonePrefab>::emplace());
        uint32 bone_node_index = model.skins[0].joints[i_bone];
        node_index_to_bone[bone_node_index] = skeleton.bones[i_bone];
    }
    uint32 ibm_index = model.skins[0].inverseBindMatrices;
    tinygltf::Accessor& ibm_accessor = model.accessors[ibm_index];
    vector<uint8> pos_data;
    _unpack_gltf_buffer(model, ibm_accessor, pos_data);
        
    for (int i = 0; i < ibm_accessor.count; i++) {
        check_else (ibm_accessor.type == TINYGLTF_TYPE_MAT4)
            continue;
            
        check_else (ibm_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT)
            continue;
            
        float* dtf = (float*) pos_data.data();

        skeleton.bones[i]->inverse_bind_matrix = {
            *(dtf + (i * 16) + 0), *(dtf + (i * 16) + 4), *(dtf + (i * 16) +  8), *(dtf + (i * 16) + 12),
            *(dtf + (i * 16) + 1), *(dtf + (i * 16) + 5), *(dtf + (i * 16) +  9), *(dtf + (i * 16) + 13),
            *(dtf + (i * 16) + 2), *(dtf + (i * 16) + 6), *(dtf + (i * 16) + 10), *(dtf + (i * 16) + 14),
            *(dtf + (i * 16) + 3), *(dtf + (i * 16) + 7), *(dtf + (i * 16) + 11), *(dtf + (i * 16) + 15)
        };
    }
    
    for (auto& [node_index, bone_ptr] : node_index_to_bone) {
        for (uint32 child_index : model.nodes[node_index].children) {
            if (node_index_to_bone.contains(child_index))
                node_index_to_bone[child_index]->parent = bone_ptr;
        }
        bone_ptr->name = model.nodes[node_index].name;
        bone_ptr->position.scale.value = v3(1,1,1);
        
        if (model.nodes[node_index].translation.size() > 0) {
            auto translation = model.nodes[node_index].translation;
            bone_ptr->position.position.value = v3{(float) translation[0], (float) translation[1], (float) translation[2]};
        }

        if (model.nodes[node_index].rotation.size() > 0) {
            bone_ptr->position.rotation.value = quat((float) model.nodes[node_index].rotation[0], (float) model.nodes[node_index].rotation[1], (float) model.nodes[node_index].rotation[2], (float) model.nodes[node_index].rotation[3]);
        }

        if (model.nodes[node_index].scale.size() > 0) {
            bone_ptr->position.scale.value = v3{(float) model.nodes[node_index].scale[0], (float) model.nodes[node_index].scale[1], (float) model.nodes[node_index].scale[2]};
        }
    }
    for (auto& [node_index, bone_ptr] : node_index_to_bone) {
        if (bone_ptr->parent.valid())
            bone_ptr->parent->length = math::length(bone_ptr->position.position.value);
    }


    vector<umap<int, KeySet>> key_sets;
    for (tinygltf::Animation& animation : model.animations) {
        for (auto& channel : animation.channels) {
            auto& sampler = animation.samplers[channel.sampler];
            auto& input_accessor = model.accessors[sampler.input];
            vector<uint8> input_buffer; // The inputs are the times of the keyframes
            _unpack_gltf_buffer(model, input_accessor, input_buffer);
            vector<float> floats;
            key_sets.resize(input_accessor.count);
            floats.resize(input_accessor.count);
            float* input_ptr = (float*) input_buffer.data();
            for (int i = 0; i < input_accessor.count; i++)
                floats[i] = *(input_ptr + i);
            
            auto& output_accessor = model.accessors[sampler.output];
            vector<uint8> output_buffer;
            _unpack_gltf_buffer(model, output_accessor, output_buffer);
            for (int i = 0; i < floats.size(); i++) {
                if (!key_sets[i].contains(channel.target_node))
                    key_sets[i].emplace(channel.target_node, KeySet{});

                if (channel.target_path == "translation") {
                    v3* output_ptr = (v3*) output_buffer.data();
                    key_sets[i][channel.target_node].position.value = *(output_ptr + i);
                } else if (channel.target_path == "rotation") {
                    quat* output_ptr = (quat*) output_buffer.data();
                    key_sets[i][channel.target_node].rotation.value = *(output_ptr + i);
                } else if (channel.target_path == "scale") {
                    v3* output_ptr = (v3*) output_buffer.data();
                    key_sets[i][channel.target_node].scale.value = *(output_ptr + i);
                }
            }
        }
    }

    if (replace_existing_pose) {
        umap<string, uint32> name_to_index;
        for (auto& entry : skeleton.pose_catalog) {
            name_to_index[entry.name] = skeleton.pose_catalog.index(entry);
        }

        for (auto& pose : key_sets) {
            // Replace existing
            if (key_sets.index(pose) < skeleton.pose_catalog.size()) {
                // Update backfill
                for (auto& [bone_index, key_set] : pose) {
                    skeleton.pose_catalog[key_sets.index(pose)].bones[node_index_to_bone[bone_index]->name] = key_set;
                }
            } else {
                string new_name = fmt_("pose_{}", key_sets.index(pose));
                auto& entry = skeleton.pose_catalog.emplace_back(Pose{new_name});
                for (auto& [bone_index, key_set] : pose) {
                    entry.bones[node_index_to_bone[bone_index]->name] = key_set;
                }
            }
        }
    } else {
        for (auto& pose : key_sets) {
            string new_name = fmt_("pose_{}", key_sets.index(pose));
            auto& entry = skeleton.pose_catalog.emplace_back(Pose{new_name});
            for (auto& [bone_index, key_set] : pose) {
                entry.bones[node_index_to_bone[bone_index]->name] = key_set;
            }
        }
    }
    
    model_cpu->skeleton = std::make_unique<SkeletonCPU>(instance_prefab(skeleton));
    model_cpu->skeleton->store_pose("default");
    
    return true;
}


void _unpack_gltf_buffer(tinygltf::Model& model, tinygltf::Accessor& accessor, vector<uint8>& output_buffer) {
    ZoneScoped;
    int                   buffer_id     = accessor.bufferView;
    uint64                   element_bsize = tinygltf::GetComponentSizeInBytes(accessor.componentType);
    tinygltf::BufferView& buffer_view   = model.bufferViews[buffer_id];
    tinygltf::Buffer&     buffer_data   = model.buffers[buffer_view.buffer];
    uint8*                   dataptr       = buffer_data.data.data() + accessor.byteOffset + buffer_view.byteOffset;
    int                   components    = tinygltf::GetNumComponentsInType(accessor.type);
    element_bsize *= components;
    uint64 stride = buffer_view.byteStride;
    if (stride == 0) {
        stride = element_bsize;
    }

    output_buffer.resize(accessor.count * element_bsize);

    for (uint32 i = 0; i < accessor.count; i++) {
        uint8* dataindex = dataptr + stride * i;
        uint8* targetptr = output_buffer.data() + element_bsize * i;

        memcpy(targetptr, dataindex, element_bsize);
    }
}

void _extract_gltf_vertices(tinygltf::Primitive& primitive, tinygltf::Model& model, vector<Vertex>& vertices) {
    ZoneScoped;
    tinygltf::Accessor& pos_accessor = model.accessors[primitive.attributes["POSITION"]];
    vertices.resize(pos_accessor.count);
    vector<uint8> pos_data;
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
    vector<uint8>          normal_data;
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
    vector<uint8>          tangent_data;
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
    vector<uint8>          uv_data;
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
        vector<uint8>          color_data;
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
        vector<uint8>          joint_data;
        _unpack_gltf_buffer(model, joint_accessor, joint_data);
        for (int i = 0; i < vertices.size(); i++) {
            uint8 components = 0;
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
            for (uint8 c = 0; c < components; c++) {
                switch (joint_accessor.componentType) {
                    case (TINYGLTF_COMPONENT_TYPE_INT): {
                        auto dtf = (int32*) joint_data.data();
                        vertices[i].bone_ids[c] = *(dtf + i * components + c);
                    } break;
                    case (TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT): {
                        auto dtf = (uint32*) joint_data.data();
                        vertices[i].bone_ids[c] = *(dtf + i * components + c);
                    } break;
                    case (TINYGLTF_COMPONENT_TYPE_SHORT): {
                        auto dtf = (int16*) joint_data.data();
                        vertices[i].bone_ids[c] = *(dtf + i * components + c);
                    } break;
                    case (TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT): {
                        auto dtf = (uint16*) joint_data.data();
                        vertices[i].bone_ids[c] = *(dtf + i * components + c);
                    } break;
                    case (TINYGLTF_COMPONENT_TYPE_BYTE): {
                        auto dtf = (int8*) joint_data.data();
                        vertices[i].bone_ids[c] = *(dtf + i * components + c);
                    } break;
                    case (TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE): {
                        auto dtf = (uint8*) joint_data.data();
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
        vector<uint8>          weight_data;
        _unpack_gltf_buffer(model, weight_accessor, weight_data);
        for (int i = 0; i < vertices.size(); i++) {
            uint8 components = 0;
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
            for (uint8 c = 0; c < components; c++) {
                switch (weight_accessor.componentType) {
                    case (TINYGLTF_COMPONENT_TYPE_FLOAT): {
                        auto dtf = (float*) weight_data.data();
                        vertices[i].bone_weights[c] = *(dtf + i * components + c);
                    } break;
                    case (TINYGLTF_COMPONENT_TYPE_DOUBLE): {
                        auto dtf = (double*) weight_data.data();
                        vertices[i].bone_weights[c] = *(dtf + i * components + c);
                    } break;
                    default:
                        assert_else("NYI" && false);
                }
            }
        }
    }
}

void _extract_gltf_indices(tinygltf::Primitive& primitive, tinygltf::Model& model, vector<uint32>& indices) {
    ZoneScoped;
    int index_accessor = primitive.indices;

    int    index_buffer   = model.accessors[index_accessor].bufferView;
    int    component_type = model.accessors[index_accessor].componentType;
    size_t indexsize      = tinygltf::GetComponentSizeInBytes(component_type);

    tinygltf::BufferView& indexview = model.bufferViews[index_buffer];
    int                   bufferidx = indexview.buffer;

    tinygltf::Buffer& buffindex = (model.buffers[bufferidx]);
    uint8*               dataptr   = buffindex.data.data() + indexview.byteOffset;
    vector<uint8>        unpackedIndices;
    _unpack_gltf_buffer(model, model.accessors[index_accessor], unpackedIndices);
    for (int i = 0; i < model.accessors[index_accessor].count; i++) {
        uint32 index;
        switch (component_type) {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                auto bfr = (uint16*) unpackedIndices.data();
                index    = *(bfr + i);
            }
            break;
            case TINYGLTF_COMPONENT_TYPE_SHORT: {
                auto bfr = (int16*) unpackedIndices.data();
                index    = *(bfr + i);
            }
            break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
                auto bfr = (uint32*) unpackedIndices.data();
                index    = *(bfr + i);
            }
            break;
            case TINYGLTF_COMPONENT_TYPE_INT: {
                int32* bfr = (int32*) unpackedIndices.data();
                index    = *(bfr + i);
            }
            break;
            default:
                log_error("Only SHORT/USHORT supported for index in GLTF convert");
        }

        indices.push_back(index);
    }

    for (uint32 i = 0; i < indices.size() / 3; i++) {
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

bool _convert_gltf_meshes(tinygltf::Model& model, const FilePath& output_folder) {
    ZoneScoped;
    for (uint32 i_mesh = 0; i_mesh < model.meshes.size(); i_mesh++) {
        auto& gltf_mesh = model.meshes[i_mesh];

        for (auto i_primitive = 0; i_primitive < gltf_mesh.primitives.size(); i_primitive++) {
            MeshCPU mesh_cpu;

            string name = _calculate_gltf_mesh_name(model, i_mesh, i_primitive);
            mesh_cpu.file_path = FilePath(output_folder.abs_path() / (name + extension(FileType_Mesh)));

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

bool _convert_gltf_materials(tinygltf::Model& model, const FilePath& output_folder) {
    ZoneScoped;
    fs::path output_folder_path = output_folder.abs_path();
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
        for (uint32 i = 0; i < 4; i++) {
            int texture_index = texture_indices[i];
            if (texture_index < 0)
                continue;
            auto image     = model.textures[texture_index];
            auto baseImage = model.images[image.source];

            if (baseImage.name == "")
                baseImage.name = texture_names[i];

            fs::path color_path = (output_folder_path / baseImage.name).string();
            color_path.replace_extension(extension(FileType_Texture));
            vuk::Format format = vuk::Format::eR8G8B8A8Srgb;

            TextureCPU texture_cpu = {
                FilePath(color_path),
                {},
                v2i{baseImage.width, baseImage.height},
                format,
                vector<uint8>(&*baseImage.image.begin(), &*baseImage.image.begin() + baseImage.image.size())
            };
            save_texture(texture_cpu);
            *texture_files[i] = texture_cpu.file_path;
        }

        material_cpu.color_tint = Color((float) pbr.baseColorFactor[0],
            (float) pbr.baseColorFactor[1],
            (float) pbr.baseColorFactor[2],
            (float) pbr.baseColorFactor[3]);
        material_cpu.emissive_tint    = material_cpu.emissive_asset_path == FilePath("white", true) ? palette::black : palette::white;
        material_cpu.roughness_factor = pbr.roughnessFactor;
        material_cpu.metallic_factor  = pbr.metallicFactor;
        material_cpu.normal_factor    = material_cpu.normal_asset_path == FilePath("white", true) ? 0.0f : 0.5f;
        fs::path material_path        = output_folder_path / (matname + extension(FileType_Material));
        material_cpu.file_path        = FilePath(material_path);

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
            matrix[n] = float(node.matrix[n]);
        }
    }
    // separate transform
    else {
        m44 translation = m44::identity();
        if (node.translation.size() > 0) {
            translation = math::translate(v3{(float) node.translation[0], (float) node.translation[1], (float) node.translation[2]});
        }

        m44 rotation = m44::identity();
        if (node.rotation.size() > 0) {
            quat rot((float) node.rotation[0], (float) node.rotation[1], (float) node.rotation[2], (float) node.rotation[3]);
            rotation = math::rotation(rot);
        }

        m44 scale = m44::identity();
        if (node.scale.size() > 0) {
            scale = math::scale(v3{(float) node.scale[0], (float) node.scale[1], (float) node.scale[2]});
        }
        matrix = (translation * rotation * scale); // * gltf_fixup;
    }

    return matrix;
};

}
