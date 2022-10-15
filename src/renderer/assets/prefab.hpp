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

struct PrefabCPU {
    string file_name;    

    struct Node {
        string name = "";
        string mesh_asset_path = "";
        string material_asset_path = "";
        m44 transform = {};

        id_ptr<Node> parent = {};
        vector<id_ptr<Node>> children = {};

        Node() = default;
    };
    
    vector<id_ptr<Node>> nodes = {};
    id_ptr<Node> root_node = id_ptr<Node>::null();

    vector<PrefabCPU> split();
};

JSON_IMPL(PrefabCPU::Node, name, mesh_asset_path, material_asset_path, transform, parent, children);

struct PrefabGPU {
    vector<slot<Renderable>> renderables;
};

void      save_prefab(const PrefabCPU&);
PrefabCPU load_prefab(const fs::path& input_path);
PrefabGPU instance_prefab(RenderScene& render_scene, const PrefabCPU&);
PrefabCPU convert_to_prefab(const fs::path& input_path, const fs::path& output_folder, const fs::path& output_name);

void inspect(PrefabCPU* prefab);

}
