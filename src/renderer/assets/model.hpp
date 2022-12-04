#pragma once

#include <filesystem>

#include "skeleton.hpp"
#include "general/string.hpp"
#include "general/vector.hpp"
#include "general/umap.hpp"
#include "general/id_ptr.hpp"
#include "general/matrix.hpp"

namespace fs = std::filesystem;

namespace spellbook {

struct MeshCPU;
struct MaterialCPU;
struct Renderable;
struct RenderScene;

struct ModelCPU {
    string file_path;

    struct Node {
        string name;
        string mesh_asset_path;
        string material_asset_path;
        m44 transform = {};

        id_ptr<SkeletonCPU> skeleton;
        
        id_ptr<Node> parent = {};
        vector<id_ptr<Node>> children = {};

        m44 calculate_transform() const;
    };
    
    vector<id_ptr<Node>> nodes = {};
    id_ptr<Node> root_node = id_ptr<Node>::null();
    vector<id_ptr<SkeletonCPU>> skeletons;

    vector<ModelCPU> split();
};

JSON_IMPL(ModelCPU::Node, name, mesh_asset_path, material_asset_path, transform, skeleton, parent, children);

struct ModelGPU {
    vector<Renderable*> renderables;
    vector<std::unique_ptr<SkeletonGPU>> skeletons;

    ModelGPU() {
        renderables = {};
        skeletons = {};
    }
    ModelGPU(const ModelGPU&) = delete;
    ModelGPU(ModelGPU&& other) = default;
    ModelGPU& operator=(ModelGPU&&) = default;
};

void     save_model(const ModelCPU&);
ModelCPU load_model(const fs::path& input_path);
ModelGPU instance_model(RenderScene&, const ModelCPU&, bool frame = false);
void     deinstance_model(RenderScene&, const ModelGPU&);
ModelCPU convert_to_model(const fs::path& input_path, const fs::path& output_folder, const fs::path& output_name);

ModelCPU quick_model(const string& name, const string& mesh, const string& material);

void inspect(ModelCPU* model);

}
