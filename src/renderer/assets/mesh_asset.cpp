#include "mesh_asset.hpp"

#include <lz4.h>

#include "math.hpp"

namespace spellbook {

MeshInfo read_mesh_info(AssetFile* file) {
    return MeshInfo(json_value(parse(file->json)));
}

void unpack_mesh(MeshInfo* info, const u8* source_buffer, u64 source_size, u8* vertices, u8* indices) {
    vector<u8> decompressed(info->vertices_bsize + info->indices_bsize);
    LZ4_decompress_safe((const char*) source_buffer, (char*) decompressed.data(), (s32) (source_size), (s32) (decompressed.size()));

    memcpy(vertices, decompressed.data(), info->vertices_bsize);
    memcpy(indices, decompressed.data() + info->vertices_bsize, info->indices_bsize);
}

AssetFile pack_mesh(MeshInfo* info, u8* vertex_data, u8* index_data) {
    AssetFile file;
    file.type[0] = 'M';
    file.type[1] = 'E';
    file.type[2] = 'S';
    file.type[3] = 'H';
    file.version = 1;

    u64 fullsize = info->vertices_bsize + info->indices_bsize;

    vector<u8> merged_buffer;
    merged_buffer.resize(fullsize);
    memcpy(merged_buffer.data(), vertex_data, info->vertices_bsize);
    memcpy(merged_buffer.data() + info->vertices_bsize, index_data, info->indices_bsize);

    // compress buffer and copy it into the file struct
    u64 compress_staging = LZ4_compressBound((s32) (fullsize));
    file.binary_blob.resize(compress_staging);
    s32 compressed_bsize = LZ4_compress_default(
        (char*) merged_buffer.data(), (char*) file.binary_blob.data(), (s32) (merged_buffer.size()), (s32) (compress_staging));
    file.binary_blob.resize(compressed_bsize);
    info->compression_mode = CompressionMode_Lz4;
    file.json              = ((json_value) *info).dump();
    return file;
}

MeshBounds calculate_bounds(Vertex* vertices, u64 count) {
    MeshBounds bounds;

    v3 min = v3(FLT_MAX);
    v3 max = v3(-FLT_MAX);

    for (u32 i = 0; i < count; i++) {
        min = math::min(min, vertices[i].position);
        max = math::max(max, vertices[i].position);
    }

    bounds.extents = (max - min) / 2.0f;
    bounds.origin  = bounds.extents + min;
    // go through the vertices again to calculate the exact bounding sphere radius
    f32 radius = 0;
    for (u32 i = 0; i < count; i++) {
        v3  offset   = vertices[i].position - bounds.origin;
        f32 distance = math::length(offset);
        radius       = math::max(radius, distance);
    }

    return bounds;
}

} 
