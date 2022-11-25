#include "astar.hpp"

#include <algorithm>
#include "lib/math.hpp"

namespace spellbook {

const vector<v2i> astar::Navigation::directions = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}, {-1, -1}, {1, 1}, {-1, 1}, {1, -1}};

astar::Node::Node(v2i init_position, shared_ptr<Node> init_parent) {
    parent   = init_parent;
    position = init_position;
    G = H = 0;
}

u32 astar::Node::get_score() {
    return G + H;
}

void astar::Navigation::remove_collision(v2i position) {
    positions.remove_if([position](v2i pos) {
        return pos == position;
    });
}

vector<v2i> astar::Navigation::find_path(v2i source, v2i target) {
    shared_ptr<Node>         current = nullptr;
    vector<shared_ptr<Node>> open_set, closed_set;
    open_set.reserve(100);
    closed_set.reserve(100);
    open_set.insert_back(make_shared<Node>(source));

    while (!open_set.empty()) {
        auto current_it = open_set.begin();
        current         = *current_it;

        for (auto it = open_set.begin(); it != open_set.end(); it++) {
            auto node = *it;
            if (node->get_score() <= current->get_score()) {
                current    = node;
                current_it = it;
            }
        }

        if (current->position == target)
            break;

        closed_set.insert_back(current);
        open_set.remove_value(*current_it);

        for (u32 i = 0; i < (diagonal ? 8 : 4); ++i) {
            v2i new_coordinate(current->position + directions[i]);
            if (_detect_collision(new_coordinate) || _find_node_on_list(closed_set, new_coordinate))
                continue;

            u32 total_cost = current->G + ((i < 4) ? 10 : 14);

            shared_ptr<Node> successor = _find_node_on_list(open_set, new_coordinate);
            if (successor == nullptr) {
                successor    = make_shared<Node>(new_coordinate, current);
                successor->G = total_cost;
                successor->H = heuristic(successor->position, target);
                open_set.insert_back(successor);
            } else if (total_cost < successor->G) {
                successor->parent = current;
                successor->G      = total_cost;
            }
        }
    }

    vector<v2i> path;
    while (current != nullptr) {
        path.insert_back(current->position);
        current = current->parent;
    }
    
    return path;
}

shared_ptr<astar::Node> astar::Navigation::_find_node_on_list(vector<shared_ptr<Node>>& nodes_, v2i position) {
    for (auto node : nodes_) {
        if (node->position == position) {
            return node;
        }
    }
    return nullptr;
}

bool astar::Navigation::_detect_collision(v2i position) {
    return std::find(positions.begin(), positions.end(), position) == positions.end();
}

}
