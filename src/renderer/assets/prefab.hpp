#pragma once

#include "vector.hpp"
#include "string.hpp"
#include "umap.hpp"
#include "slotmap.hpp"
#include "id_ptr.hpp"

#include "matrix.hpp"

namespace spellbook {

struct MeshCPU;
struct MaterialCPU;
struct Renderable;
struct RenderScene;

struct PrefabCPU {
    string file_name;    

    struct Node {
        string name = {};
        // hashed MeshCPU/MaterialCPU, can use renderer.mesh_aliases, etc.
        u64 mesh = {};
        u64 material = {};
        m44 transform = {};

        id_ptr<Node> parent = {};
        vector<id_ptr<Node>> children = {};

        Node() = default;
    };

    vector<MaterialCPU> materials = {};
    vector<MeshCPU> meshes = {};
    vector<id_ptr<Node>> nodes = {};
    id_ptr<Node> root_node = id_ptr<Node>::null();

    vector<PrefabCPU> split();
};

JSON_IMPL(PrefabCPU::Node, name, mesh, material, transform, parent, children);

struct PrefabGPU {
    vector<slot<Renderable>> renderables;
};

void      save_prefab(const PrefabCPU&);
PrefabCPU load_prefab(const string_view file_name);
PrefabGPU instance_prefab(RenderScene& render_scene, const PrefabCPU&);
PrefabCPU convert_to_prefab(const string& input_file, const string& output_folder);

}