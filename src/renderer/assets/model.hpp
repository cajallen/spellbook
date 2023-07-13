#pragma once

#include <filesystem>

#include "skeleton.hpp"
#include "general/string.hpp"
#include "general/vector.hpp"
#include "general/umap.hpp"
#include "general/id_ptr.hpp"
#include "general/math/matrix.hpp"
#include "game/game_file.hpp"

namespace fs = std::filesystem;

namespace spellbook {

struct MeshCPU;
struct MaterialCPU;
struct StaticRenderable;
struct Renderable;
struct RenderScene;
struct SkeletonCPU;
struct SkeletonGPU;

struct ModelCPU {
    string file_path;
    vector<string> dependencies;

    struct Node {
        string name;
        string mesh_asset_path;
        string material_asset_path;
        m44 transform = {};
        
        id_ptr<Node> parent = {};
        vector<id_ptr<Node>> children = {};
        
        m44 cached_transform;
        
        void cache_transform();
    };
    
    vector<id_ptr<Node>> nodes = {};
    id_ptr<Node> root_node = id_ptr<Node>::null();
    std::unique_ptr<SkeletonCPU> skeleton;

    vector<ModelCPU> split();

    ModelCPU() = default;
    ModelCPU(ModelCPU&&) = default;
    ModelCPU(const ModelCPU& oth);

    ModelCPU& operator = (const ModelCPU& oth);
};

JSON_IMPL(ModelCPU::Node, name, mesh_asset_path, material_asset_path, transform, parent, children);

struct ModelGPU {
    umap<ModelCPU::Node*, Renderable*> renderables;
    std::unique_ptr<SkeletonGPU> skeleton;

    ModelGPU() {
        renderables = {};
        skeleton = {};
    }
    ModelGPU(const ModelGPU&) = delete;
    ModelGPU(ModelGPU&& other) = default;
    ModelGPU& operator=(ModelGPU&&) = default;
};

template <>
bool     save_asset(const ModelCPU& asset_file);
template <>
ModelCPU& load_asset(const string& input_path, bool assert_exist, bool clear_cache);

ModelGPU instance_model(RenderScene&, const ModelCPU&, bool frame = false);
vector<StaticRenderable*> instance_static_model(RenderScene&, const ModelCPU&);
void     deinstance_model(RenderScene&, const ModelGPU&);
void     deinstance_static_model(RenderScene&, const vector<StaticRenderable*>&);
ModelCPU convert_to_model(const fs::path& input_path, const fs::path& output_folder, const fs::path& output_name, bool y_up = true, bool replace_existing_poses = false);

ModelCPU quick_model(const string& name, const string& mesh, const string& material);

bool inspect(ModelCPU* model, RenderScene* render_scene = nullptr);

}
