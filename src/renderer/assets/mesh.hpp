#pragma once

#include <vuk/Types.hpp>
#include <vuk/Buffer.hpp>

#include "general/vector.hpp"
#include "general/string.hpp"
#include "general/file/json.hpp"
#include "general/file/file_path.hpp"
#include "general/file/resource.hpp"
#include "renderer/vertex.hpp"

namespace spellbook {

struct MeshInfo {
    uint32 vertices_bsize = 0;
    uint32 indices_bsize  = 0;
    uint32 index_bsize    = 0;
};

struct MeshBounds {
    bool valid;
    v3   extents;
    v3   origin;
    float  radius;
};

struct MeshCPU : Resource {
    vector<Vertex> vertices;
    vector<uint32> indices;

    MeshBounds bounds;

    void fix_tangents();

    static constexpr string_view extension() { return ".sbamsh"; }
    static constexpr string_view dnd_key() { return "DND_MESH"; }
    static FilePath folder() { return get_resource_folder(); }
    static std::function<bool(const FilePath&)> path_filter() { return [](const FilePath& path) { return path.extension() == MeshCPU::extension(); }; }
};
JSON_IMPL(MeshInfo, vertices_bsize, indices_bsize, index_bsize);
JSON_IMPL(MeshBounds, valid, extents, origin, radius);
JSON_IMPL(MeshCPU, bounds);

struct MeshGPU {
    vuk::Unique<vuk::Buffer> vertex_buffer;
    vuk::Unique<vuk::Buffer> index_buffer;

    uint32 vertex_count;
    uint32 index_count;

    bool frame_allocated;
};

MeshCPU load_mesh(const FilePath& file_path);
void    save_mesh(const MeshCPU& mesh_cpu);
uint64 upload_mesh(const MeshCPU&, bool frame_allocation = false);

}
