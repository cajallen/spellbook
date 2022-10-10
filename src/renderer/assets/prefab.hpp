#pragma once

#include "vector.hpp"
#include "string.hpp"
#include "umap.hpp"
#include "slotmap.hpp"
#include "id_ptr.hpp"

#include "matrix.hpp"

struct MeshCPU;
struct MaterialCPU;
struct Renderable;
struct RenderScene;

namespace spellbook {

struct PrefabCPU {
    string file_name;    

    struct Node {
        string name = {};
        string mesh = {};
        string material = {};
        m44 transform = {};

        id_ptr<Node> parent = {};
        uset<id_ptr<Node>> children = {};

        Node() = default;
        JSON_IMPL(Node, name, mesh, material, transform, parent, children);
    };

    vector<MaterialCPU> materials = {};
    vector<MeshCPU> meshes = {};
    vector<id_ptr<Node>> nodes = {};
    id_ptr<Node> root_node = id_ptr<Node>::null();

    vector<PrefabCPU> split();
};

struct PrefabGPU {
    vector<slot<Renderable>> renderables;
};

void      save_prefab(const PrefabCPU&);
PrefabCPU load_prefab(const string_view file_name);
PrefabGPU instance_prefab(RenderScene& render_scene, const PrefabCPU&);
PrefabCPU convert_to_prefab(const string& input_file, const string& output_folder);

}
