#pragma once

#include "string.hpp"

#include "json.hpp"

#include "renderer/vertex.hpp"
#include "renderer/assets/asset_loader.hpp"
#include "renderer/assets/mesh.hpp"

namespace spellbook {

/*
 * Asset files are special file types that contain header data at the start and a binary blob at the end.
 *
 * To load a normally serialized file, we load a file into json and construct the object out of the json
 *
 * To load an asset file, we load a file into an AssetFile, convert it to it's info type (containing
 * binary info) and json type (containing game info), and construct the object out of those two.
 */

struct MeshInfo {
    CompressionMode compression_mode = {};
    u64             vertices_bsize   = 0;
    u64             indices_bsize    = 0;
    u32             index_bsize      = 0;

    MeshInfo() = default;
    JSON_IMPL(MeshInfo, compression_mode, vertices_bsize, indices_bsize, index_bsize);
};

MeshInfo   read_mesh_info(AssetFile* file);
void       unpack_mesh(MeshInfo* info, const u8* source_buffer, u64 source_size, u8* vertices, u8* indices);
void       save_mesh(MeshCPU& mesh_cpu);

}
