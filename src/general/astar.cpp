#include "astar.hpp"

#include <algorithm>
#include "math.hpp"
#include "editor/console.hpp"
#include "extension/fmt.hpp"
#include "extension/fmt_geometry.hpp"

namespace spellbook {

const vector<v3i> astar::Navigation::directions = {{0, 1,0}, {1, 0, 0}, {0, -1, 0}, {-1, 0, 0}, {-1, -1, 0}, {1, 1, 0}, {-1, 1, 0}, {1, -1, 0}};

astar::Node::Node(v3i init_position, const shared_ptr<Node>& init_parent) {
    parent   = init_parent;
    position = init_position;
    G = H = 0;
}

u32 astar::Node::get_score() const {
    return G + H;
}

void astar::Navigation::clear() {
    path_solids.clear();
    off_road_solids.clear();
    ramps.clear();
}


vector<v3i> astar::Navigation::find_path(v3i source, v3i target, float tolerance) {
    shared_ptr<Node>         current = nullptr;
    vector<shared_ptr<Node>> open_set, closed_set;
    open_set.reserve(100);
    closed_set.reserve(100);
    open_set.push_back(make_shared<Node>(source));

    bool found_target = false;
    while (!open_set.empty()) {
        u32 current_i = 0;
        current = open_set[current_i];
        for (u32 i = 1; i < open_set.size(); ++i) {
            auto node = open_set[i];
            if (node->get_score() <= current->get_score()) {
                current   = node;
                current_i = i;
            }
        }

        if (math::distance(v3(current->position), v3(target)) < tolerance) {
            found_target = true;
            break;
        }

        closed_set.push_back(current);
        open_set.remove_index(current_i, true);
        
        for (auto [new_pos, cost] : _get_neighbors(current->position)) {
            if (_find_node_on_list(closed_set, new_pos))
                continue;

            u32 total_cost = current->G + cost;

            shared_ptr<Node> successor = _find_node_on_list(open_set, new_pos);
            if (successor == nullptr) {
                successor    = make_shared<Node>(new_pos, current);
                successor->G = total_cost;
                successor->H = heuristic(successor->position, target);
                open_set.push_back(successor);
            } else if (total_cost < successor->G) {
                successor->parent = current;
                successor->G      = total_cost;
            }
        }
    }
    
    if (!found_target) {
        float current_dist = math::distance(v3(current->position), v3(target));
        for (shared_ptr<Node>& potential_node : closed_set) {
            float potential_dist = math::distance(v3(potential_node->position), v3(target));
            if (potential_dist < current_dist) {
                current = potential_node;
                current_dist = potential_dist;
            }
        }
    }
    
    vector<v3i> path;
    while (current != nullptr) {
        path.push_back(current->position);
        current = current->parent;
    }
    
    return path;
}

shared_ptr<astar::Node> astar::Navigation::_find_node_on_list(vector<shared_ptr<Node>>& nodes_, v3i position) {
    for (auto node : nodes_) {
        if (node->position == position) {
            return node;
        }
    }
    return nullptr;
}

astar::Navigation::TileType astar::Navigation::_position_viable(v3i position) {
    bool occupied = path_solids.get(position) || off_road_solids.get(position);
    if (occupied)
        return TileType_Empty;
    if (path_solids.get(position + v3i(0,0,-1)))
        return TileType_Path;
    if (off_road_solids.get(position + v3i(0,0,-1)))
        return TileType_OffRoad;
    return TileType_Empty;
}

int astar::Navigation::_type_cost(TileType type) {
    switch (type) {
        case (TileType_Path): return 10;
        case (TileType_OffRoad): return 400;
        default: return 100000;
    }
}


vector<std::pair<v3i, u32>> astar::Navigation::_get_neighbors(v3i position) {
    vector<std::pair<v3i, u32>> neighbors = {};
    for (Direction neighbor : {Direction_PosX, Direction_PosY, Direction_NegX, Direction_NegY}) {
        v3i neighbor_pos = position + direction_to_vec(neighbor);
        TileType neighbor_tile = _position_viable(neighbor_pos);
        if (ramps.contains(neighbor_pos)) {
            if (ramps[neighbor_pos] == neighbor)
                neighbors.push_back({neighbor_pos + v3i(0, 0, 1), 11});
        }
        else if (ramps.contains(neighbor_pos + v3i(0,0,-1))) {
            if (ramps[neighbor_pos + v3i(0,0,-1)] == flip_direction(neighbor))
                neighbors.push_back({neighbor_pos + v3i(0,0,-1), 11});
        }
        else if (neighbor_tile != TileType_Empty)
            neighbors.push_back({neighbor_pos, _type_cost(neighbor_tile)});
    }
    string vector_string = "{";
    for (auto [neighbor_pos, cost] : neighbors) {
        vector_string += fmt_("{}", neighbor_pos, ",");
    }
    vector_string += "}";
    console({.str=fmt_("Built neighbors for {}: {}", position, vector_string), .frame_tags = {"astar"}});
    return neighbors;
}

}
