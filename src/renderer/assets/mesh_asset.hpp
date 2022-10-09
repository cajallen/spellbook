#pragma once

#include "string.hpp"

#include "json.hpp"

#include "renderer/vertex.hpp"
#include "renderer/assets/asset_loader.hpp"

namespace spellbook {

/*
 * Asset files are special file types that contain header data at the start and a binary blob at the end.
 *
 * To load a normally serialized file, we load a file into json and construct the object out of the json
 *
 * To load an asset file, we load a file into an AssetFile, convert it to it's info type and json type,
 * and construct the object out of those two.
 */

struct MeshBounds {
    v3  origin  = v3();
    v3  extents = v3();
    f32 radius  = 0.0f;

    MeshBounds() = default;
    JSON_IMPL(MeshBounds, origin, extents, radius);
};

struct MeshInfo {
    CompressionMode compression_mode = {};
    MeshBounds      bounds           = {};
    u64             vertices_bsize   = 0;
    u64             indices_bsize    = 0;
    u32             index_bsize      = 0;

    MeshInfo() = default;
    JSON_IMPL(MeshInfo, compression_mode, vertices_bsize, indices_bsize, index_bsize, bounds);
};

MeshInfo   read_mesh_info(AssetFile* file);
void       unpack_mesh(MeshInfo* info, const u8* source_buffer, u64 source_size, u8* vertex_buffer, u8* index_buffer);
AssetFile  pack_mesh(MeshInfo* info, u8* vertex_data, u8* index_data);
MeshBounds calculate_bounds(Vertex* vertices, u64 count);

}
