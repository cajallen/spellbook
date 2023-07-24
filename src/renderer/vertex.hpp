#pragma once

#include <vuk/CommandBuffer.hpp>

#include "general/math/geometry.hpp"

namespace spellbook {

struct Vertex {
	v3 position;
	v3 normal = {};
	v3 tangent = {};
	v3 color = {};
	v2 uv = {};
    v4i bone_ids = {-1, -1, -1, -1};
    v4 bone_weights = {};

    static inline vuk::Packed get_format() {
        return vuk::Packed {
            vuk::Format::eR32G32B32Sfloat, // position
            vuk::Format::eR32G32B32Sfloat, // normal
            vuk::Format::eR32G32B32Sfloat, // tangent
            vuk::Format::eR32G32B32Sfloat, // color
            vuk::Format::eR32G32Sfloat,    // uv
            vuk::Format::eR32G32B32A32Sint, // bone id
            vuk::Format::eR32G32B32A32Sfloat // bone weight
        };
    }

    static inline vuk::Packed get_widget_format() {
        return vuk::Packed {
            vuk::Format::eR32G32B32Sfloat, // position
            vuk::Ignore{ sizeof(Vertex::normal)  + sizeof(Vertex::tangent) }, // normal
            vuk::Format::eR32G32B32Sfloat, // color
            vuk::Ignore{ sizeof(Vertex::uv)  + sizeof(Vertex::bone_ids) + sizeof(bone_weights) }
        };
    }
};

}
