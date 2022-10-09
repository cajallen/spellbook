#include "prefab.hpp"

#include "renderer/renderer.hpp"
#include "renderer/render_scene.hpp"

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
        json_node["id"] = make_shared<json_value>(node);
        json_nodes.insert_back((json_value) json_node);
    }
    j["nodes"] = make_shared<json_value>(json_nodes);
    file_dump(j, prefab.file_name);
}

PrefabCPU load_prefab(const string_view file_name) {
    PrefabCPU prefab;
    json j = parse_file(file_name);
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
        auto new_renderable = render_scene.add_renderable(Renderable(node.name, node.mesh, node.material, node.transform));
        prefab_gpu.renderables.insert_back(new_renderable);
    }
    
    return prefab_gpu;
}


}