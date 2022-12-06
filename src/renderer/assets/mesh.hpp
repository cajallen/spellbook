#pragma once

#include <vuk/Types.hpp>
#include <vuk/Buffer.hpp>

#include "general/vector.hpp"
#include "general/string.hpp"
#include "general/json.hpp"
#include "renderer/vertex.hpp"
#include "renderer/assets/skeleton.hpp"


namespace spellbook {

struct MeshBounds {
    bool valid;
    v3   extents;
    v3   origin;
    f32  radius;
};

struct MeshCPU {
    string         file_path;
    
    vector<Vertex> vertices;
    vector<u32>    indices;

    MeshBounds bounds;

    void fix_tangents();
};
JSON_IMPL(MeshBounds, valid, extents, origin, radius);
JSON_IMPL(MeshCPU, bounds);

struct MeshGPU {
    vuk::Unique<vuk::Buffer> vertex_buffer;
    vuk::Unique<vuk::Buffer> index_buffer;

    u32 vertex_count;
    u32 index_count;

    bool frame_allocated;
};

void inspect(MeshGPU* mesh);

}
