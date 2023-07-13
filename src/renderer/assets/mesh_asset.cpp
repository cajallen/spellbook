#include "mesh_asset.hpp"

#include <lz4.h>

#include "game/game.hpp"
#include "game/game_file.hpp"
#include "general/logger.hpp"

namespace spellbook {

MeshCPU load_mesh(const string& file_name) {
    // TODO: CompressionMode
    AssetFile& asset_file = game.asset_system.load_asset(to_resource_path(file_name).string());

    constexpr array expected_type = {'M','S','H'};
    check_else(asset_file.version == 3 && asset_file.type == expected_type)
        return {};
    
    MeshInfo mesh_info = from_jv<MeshInfo>(*asset_file.asset_json["mesh_info"]);
    MeshCPU mesh_cpu = from_jv<MeshCPU>(*asset_file.asset_json["mesh_cpu"]);
    mesh_cpu.file_path = file_name;
    
    vector<uint8> decompressed;
    decompressed.resize(mesh_info.vertices_bsize + mesh_info.indices_bsize);
    LZ4_decompress_safe((const char*) asset_file.binary_blob.data(), (char*) decompressed.data(), asset_file.binary_blob.size(), (int32) (decompressed.size()));

    mesh_cpu.vertices.rebsize(mesh_info.vertices_bsize);
    mesh_cpu.indices.rebsize(mesh_info.indices_bsize);

    memcpy(mesh_cpu.vertices.data(), decompressed.data(), mesh_info.vertices_bsize);
    memcpy(mesh_cpu.indices.data(), decompressed.data() + mesh_info.vertices_bsize, mesh_info.indices_bsize);

    return mesh_cpu;
}

void save_mesh(const MeshCPU& mesh_cpu) {
    AssetFile file;
    file.file_name = mesh_cpu.file_path;
    file.type = {'M','S','H'};
    file.version = 3;

    MeshInfo mesh_info;
    mesh_info.vertices_bsize = mesh_cpu.vertices.bsize();
    mesh_info.indices_bsize  = mesh_cpu.indices.bsize();
    mesh_info.index_bsize    = sizeof(uint32);
    
    vector<uint8> merged_buffer;
    merged_buffer.resize(uint32(mesh_info.vertices_bsize + mesh_info.indices_bsize));
    memcpy(merged_buffer.data(), mesh_cpu.vertices.data(), mesh_info.vertices_bsize);
    memcpy(merged_buffer.data() + mesh_info.vertices_bsize, mesh_cpu.indices.data(), mesh_info.indices_bsize);

    int32 compress_staging = LZ4_compressBound(int32(mesh_info.vertices_bsize + mesh_info.indices_bsize));
    file.binary_blob.resize(compress_staging);
    int32 compressed_bsize = LZ4_compress_default((char*) merged_buffer.data(), (char*) file.binary_blob.data(), int32(merged_buffer.size()), int32(compress_staging));
    file.binary_blob.resize(compressed_bsize);
    mesh_info.compression_mode = CompressionMode_Lz4;

    json j;
    j["mesh_cpu"] = make_shared<json_value>(to_jv(mesh_cpu));
    j["mesh_info"] = make_shared<json_value>(to_jv(mesh_info));
    file.asset_json = j;

    save_asset_file(file);
}

} 
