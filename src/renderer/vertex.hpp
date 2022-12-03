#pragma once

#include <vuk/CommandBuffer.hpp>

#include "lib/geometry.hpp"

namespace spellbook {

struct Vertex {
	v3 position;
	v3 normal = {};
	v3 tangent = {};
	v3 color = {};
	v2 uv = {};
    v4i bone_ids = {-1, -1, -1, -1};
    v4 bone_weights;

    static vuk::Packed get_format();
};

}
