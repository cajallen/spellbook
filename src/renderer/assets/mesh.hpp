#pragma once

#include <vuk/Types.hpp>
#include <vuk/Buffer.hpp>

#include "lib/vector.hpp"
#include "lib/string.hpp"
#include "lib/json.hpp"
#include "renderer/vertex.hpp"


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
    vuk::Unique<vuk::BufferGPU> vertex_buffer;
    vuk::Unique<vuk::BufferGPU> index_buffer;

    u32 vertex_count;
    u32 index_count;
};

void inspect(MeshGPU* mesh);

}
