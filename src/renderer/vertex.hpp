#pragma once

#include "geometry.hpp"

namespace spellbook {

struct Vertex {
	v3 position;
	v3 normal = v3(0,0,1);
	v3 tangent = {};
	v3 color = {};
	v2 uv = {};
};

}
