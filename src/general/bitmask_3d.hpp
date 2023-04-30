#pragma once

#include "general/umap.hpp"
#include "general/geometry.hpp"
#include "general/math.hpp"

namespace spellbook {

struct Bitmask3D {
    umap<v3i, u64> chunks;

    void set(v3i pos, bool on = true) ;
    bool get(v3i pos);
    v3i rough_min();
    v3i rough_max();
    bool ray_intersection(ray3 ray, v3& pos, v3i& cube);
    void clear();
};

}