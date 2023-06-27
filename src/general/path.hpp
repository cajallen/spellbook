#pragma once

#include "general/vector.hpp"
#include "general/geometry.hpp"

namespace spellbook {

struct Path {
    vector<v3> waypoints;
    s32 reached = INT_MAX;

    Path() {}
    Path(const vector<v3>& wps) : waypoints(wps), reached(wps.size()) {}
    Path(vector<v3>&& wps) : waypoints(wps), reached(wps.size()) {}

    v3 get_start() const;
    v3 get_destination() const;
    bool valid() const;

    v3 get_real_target(const v3& current_pos);
};

}