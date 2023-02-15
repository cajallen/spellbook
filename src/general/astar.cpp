#include "astar.hpp"

#include <algorithm>
#include "math.hpp"
#include "editor/console.hpp"
#include "extension/fmt.hpp"
#include "extension/fmt_geometry.hpp"

namespace spellbook {

const vector<v3i> astar::Navigation::directions = {{0, 1,0}, {1, 0, 0}, {0, -1, 0}, {-1, 0, 0}, {-1, -1, 0}, {1, 1, 0}, {-1, 1, 0}, {1, -1, 0}};

astar::Node::Node(v3i init_position, shared_ptr<Node> init_parent) {
    parent   = init_parent;
    position = init_position;
    G = H = 0;
}

u32 astar::Node::get_score() {
    return G + H;
}

vector<v3i> astar::Navigation::find_path(v3i source, v3i target) {
    shared_ptr<Node>         current = nullptr;
    vector<shared_ptr<Node>> open_set, closed_set;
    open_set.reserve(100);
    closed_set.reserve(100);
    open_set.push_back(make_shared<Node>(source));

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

        if (current->position == target)
            break;

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

bool astar::Navigation::_position_viable(v3i position) {
    bool occupied = solids.get(position);
    bool has_floor = solids.get(position + v3i(0,0,-1));
    return !occupied && has_floor;
}

vector<std::pair<v3i, u32>> astar::Navigation::_get_neighbors(v3i position) {
    vector<std::pair<v3i, u32>> neighbors = {};
    for (Direction neighbor : {Direction_PosX, Direction_PosY, Direction_NegX, Direction_NegY}) {
        v3i neighbor_pos = position + direction_to_vec(neighbor);
        if (ramps.contains(neighbor_pos)) {
            if (ramps[neighbor_pos] == neighbor)
                neighbors.push_back({neighbor_pos + v3i(0, 0, 1), 11});
        }
        else if (ramps.contains(neighbor_pos + v3i(0,0,-1))) {
            if (ramps[neighbor_pos + v3i(0,0,-1)] == flip_direction(neighbor))
                neighbors.push_back({neighbor_pos + v3i(0,0,-1), 11});
        }
        else if (_position_viable(neighbor_pos))
            neighbors.push_back({neighbor_pos, 10});
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
