#include "mesh_asset.hpp"

#include <lz4.h>

#include "math.hpp"

namespace spellbook {

MeshCPU load_mesh(const string& file_name) {
    // TODO: CompressionMode
    AssetFile asset_file = load_asset_file(file_name);

    constexpr array expected_type = {'M','S','H'};
    assert_else(asset_file.version == 2 && asset_file.type == expected_type)
        return {};
    
    MeshInfo mesh_info = MeshInfo(*asset_file.asset_json["mesh_info"]);
    MeshCPU mesh_cpu = MeshCPU(*asset_file.asset_json["mesh_cpu"]);
    
    vector<u8> decompressed(mesh_info.vertices_bsize + mesh_info.indices_bsize);
    LZ4_decompress_safe((const char*) asset_file.binary_blob.data(), (char*) decompressed.data(), asset_file.binary_blob.size(), (s32) (decompressed.size()));

    memcpy(mesh_cpu.vertices.data(), decompressed.data(), mesh_info.vertices_bsize);
    memcpy(mesh_cpu.indices.data(), decompressed.data() + mesh_info.vertices_bsize, mesh_info.indices_bsize);

    return mesh_cpu;
}

void save_mesh(MeshCPU& mesh_cpu) {
    AssetFile file;
    file.file_name = mesh_cpu.file_name;
    file.type = {'M','S','H'};
    file.version = 2;

    MeshInfo mesh_info;
    mesh_info.vertices_bsize = mesh_cpu.vertices.size() * sizeof(Vertex);
    mesh_info.indices_bsize  = mesh_cpu.indices.size() * sizeof(u32);
    mesh_info.index_bsize    = sizeof(u32);
    
    vector<u8> merged_buffer;
    merged_buffer.resize(u32(mesh_info.vertices_bsize + mesh_info.indices_bsize));
    memcpy(merged_buffer.data(), mesh_cpu.vertices.data(), mesh_info.vertices_bsize);
    memcpy(merged_buffer.data() + mesh_info.vertices_bsize, mesh_cpu.indices.data(), mesh_info.indices_bsize);

    s32 compress_staging = LZ4_compressBound(s32(mesh_info.vertices_bsize + mesh_info.indices_bsize));
    file.binary_blob.resize(compress_staging);
    s32 compressed_bsize = LZ4_compress_default((char*) merged_buffer.data(), (char*) file.binary_blob.data(), s32(merged_buffer.size()), s32(compress_staging));
    file.binary_blob.resize(compressed_bsize);
    mesh_info.compression_mode = CompressionMode_Lz4;

    json j;
    j["mesh_cpu"] = make_shared<json_value>(mesh_cpu);
    j["mesh_info"] = make_shared<json_value>(mesh_info);
    file.asset_json = j;

    save_asset_file(file);
}

} 
