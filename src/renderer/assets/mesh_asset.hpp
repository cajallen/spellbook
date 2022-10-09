#pragma once

#include "asset_loader.hpp"
#include "renderer/vertex.hpp"
#include "json.hpp"

#include "string.hpp"

namespace spellbook {

struct MeshBounds {
    v3  origin = v3();
    v3  extents = v3();
    f32 radius = 0.0f;

    MeshBounds() = default;
    JSON_IMPL(MeshBounds, origin, extents, radius);
};

struct MeshInfo {
    string     original_file = {};
    u32        compression_mode = 0;
    u64        vertices_bsize = 0;
    u64        indices_bsize = 0;
    u32        index_bsize = 0;
    MeshBounds bounds = {};

    MeshInfo() = default;
    JSON_IMPL(MeshInfo, original_file, compression_mode, vertices_bsize, indices_bsize, index_bsize, bounds);
};

MeshInfo   read_mesh_info(AssetFile* file);
void       unpack_mesh(MeshInfo* info, const u8* source_buffer, u64 source_size, u8* vertex_buffer, u8* index_buffer);
AssetFile  pack_mesh(MeshInfo* info, u8* vertex_data, u8* index_data);
MeshBounds calculate_bounds(Vertex* vertices, u64 count);

}
