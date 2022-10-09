#pragma once

#include "vector.hpp"
#include <functional>

#include "geometry.hpp"
#include "math.hpp"

namespace spellbook::astar {
using HeuristicFunction = std::function<u32(v2i, v2i)>;

struct Node {
    u32   G, H;
    v2i   position;
    Node* parent;

    Node(v2i coord_, Node* parent_ = nullptr);
    u32 get_score();
};

struct Navigation {
    bool  _detect_collision(v2i position);
    Node* _find_node_on_list(vector<Node*>& nodes, v2i position);
    void  _release_nodes(vector<Node*>& nodes);

    // includes start/end, reverse order (target first)
    vector<v2i> find_path(v2i source, v2i target);
    void        remove_collision(v2i position);

    HeuristicFunction heuristic = [](v2i start, v2i end) {
        v2i delta = end - start;
        return 10 * math::abs(delta.x) + 10 * math::abs(delta.y);
    };
    vector<v2i> positions = {};
    bool        diagonal  = false;

    static const vector<v2i> directions;
};

}

