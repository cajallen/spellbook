#pragma once

#include <vuk/Types.hpp>
#include <vuk/Buffer.hpp>

#include "general/vector.hpp"
#include "general/string.hpp"
#include "general/json.hpp"
#include "renderer/vertex.hpp"
#include "general/file_path.hpp"

namespace spellbook {

struct MeshBounds {
    bool valid;
    v3   extents;
    v3   origin;
    float  radius;
};

struct MeshCPU {
    FilePath file_path;
    
    vector<Vertex> vertices;
    vector<uint32> indices;

    MeshBounds bounds;

    void fix_tangents();
};
JSON_IMPL(MeshBounds, valid, extents, origin, radius);
JSON_IMPL(MeshCPU, bounds);

struct MeshGPU {
    vuk::Unique<vuk::Buffer> vertex_buffer;
    vuk::Unique<vuk::Buffer> index_buffer;

    uint32 vertex_count;
    uint32 index_count;

    bool frame_allocated;
};

uint64 upload_mesh(const MeshCPU&, bool frame_allocation = false);

}
