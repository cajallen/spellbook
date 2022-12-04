#pragma once

#include "general/string.hpp"
#include "general/json.hpp"
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
    u32             vertices_bsize   = 0;
    u32             indices_bsize    = 0;
    u32             index_bsize      = 0;
};

JSON_IMPL(MeshInfo, compression_mode, vertices_bsize, indices_bsize, index_bsize);

MeshCPU load_mesh(const string& file_name);
void    save_mesh(const MeshCPU& mesh_cpu);

}
