#pragma once

#include <filesystem>

#include <filesystem>
#include "vector.hpp"
#include "string.hpp"
#include "umap.hpp"
#include "slotmap.hpp"
#include "id_ptr.hpp"

#include "matrix.hpp"

namespace fs = std::filesystem;

namespace spellbook {

struct MeshCPU;
struct MaterialCPU;
struct Renderable;
struct RenderScene;

struct ModelCPU {
    string file_name;    

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
    vector<slot<Renderable>> renderables;
};

void     save_model(const ModelCPU&);
ModelCPU load_model(const fs::path& input_path);
ModelGPU instance_model(RenderScene&, const ModelCPU&);
void     deinstance_model(RenderScene&, const ModelGPU&);
ModelCPU convert_to_model(const fs::path& input_path, const fs::path& output_folder, const fs::path& output_name);

void inspect(ModelCPU* model);

}
