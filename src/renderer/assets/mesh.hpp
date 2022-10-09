#pragma once

#include <vuk/Types.hpp>
#include <vuk/Buffer.hpp>

#include "vector.hpp"
#include "string.hpp"

#include "renderer/vertex.hpp"
#include "renderer/render_scene.hpp"

namespace spellbook {

struct MeshBounds {
    bool valid;
    v3   extents;
    v3   origin;
    f32  radius;
};

struct MeshCPU {
    string         name;
    string         file_name;
    vector<Vertex> vertices;
    vector<u32>    indices;

    MeshBounds bounds;

    JSON_IMPL(MeshCPU, name);
};

struct MeshGPU {
    vuk::Unique<vuk::BufferGPU> vertex_buffer;
    vuk::Unique<vuk::BufferGPU> index_buffer;

    u32 vertex_count;
    u32 index_count;
};

void inspect(MeshGPU* mesh);

void    save_mesh(const MeshCPU&);
MeshCPU load_mesh(const string_view file_name);

}
