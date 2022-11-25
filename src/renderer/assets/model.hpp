#pragma once

#include <filesystem>

#include "lib/string.hpp"
#include "lib/vector.hpp"
#include "lib/umap.hpp"
#include "lib/id_ptr.hpp"
#include "lib/matrix.hpp"

namespace fs = std::filesystem;

namespace spellbook {

struct MeshCPU;
struct MaterialCPU;
struct Renderable;
struct RenderScene;

struct ModelCPU {
    string file_path;    

    struct Node {
        string name = "";
        string mesh_asset_path = "";
        string material_asset_path = "";
        m44 transform = {};

        id_ptr<Node> parent = {};
        vector<id_ptr<Node>> children = {};

        m44 calculate_transform() const;
    };
    
    vector<id_ptr<Node>> nodes = {};
    id_ptr<Node> root_node = id_ptr<Node>::null();

    vector<ModelCPU> split();
};

JSON_IMPL(ModelCPU::Node, name, mesh_asset_path, material_asset_path, transform, parent, children);

struct ModelGPU {
    vector<Renderable*> renderables;
};

void     save_model(const ModelCPU&);
ModelCPU load_model(const fs::path& input_path);
ModelGPU instance_model(RenderScene&, const ModelCPU&, bool frame = false);
void     deinstance_model(RenderScene&, const ModelGPU&);
ModelCPU convert_to_model(const fs::path& input_path, const fs::path& output_folder, const fs::path& output_name);

ModelCPU quick_model(const string& name, const string& mesh, const string& material);

void inspect(ModelCPU* model);

}
