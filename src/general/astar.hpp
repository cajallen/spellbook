#pragma once

#include <functional>

#include "vector.hpp"
#include "geometry.hpp"
#include "math.hpp"
#include "bitmask_3d.hpp"

namespace spellbook::astar {
using HeuristicFunction = std::function<u32(v3i, v3i)>;

struct Node {
    u32   G, H;
    v3i   position;
    shared_ptr<Node> parent;

    Node(v3i coord_, shared_ptr<Node> parent_ = nullptr);
    u32 get_score();
};

struct Navigation {
    enum TileType {
        TileType_Empty,
        TileType_Path,
        TileType_OffRoad
    };
    TileType  _position_viable(v3i position);
    int _type_cost(TileType);
    shared_ptr<Node> _find_node_on_list(vector<shared_ptr<Node>>& nodes, v3i position);
    vector<std::pair<v3i, u32>> _get_neighbors(v3i position);
    
    // includes start/end, reverse order (target first)
    vector<v3i> find_path(v3i source, v3i target, float tolerance = 0.1f);

    HeuristicFunction heuristic = [](v3i start, v3i end) {
        v3i delta = end - start;
        return 10 * math::abs(delta.x) + 10 * math::abs(delta.y);
    };
    Bitmask3D path_solids = {};
    Bitmask3D off_road_solids = {};
    umap<v3i, Direction> ramps = {};
    bool diagonal = false;

    static const vector<v3i> directions;
};

}

