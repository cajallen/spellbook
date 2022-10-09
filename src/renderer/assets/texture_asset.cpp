#include "texture_asset.hpp"

#include <lz4.h>
#include <iostream>

#include "console.hpp"
#include "lib_ext/fmt_geometry.hpp"

namespace spellbook {

TextureInfo read_texture_info(AssetFile* file) {
    return TextureInfo(json_value(parse(file->json)));
}

void unpack_texture(TextureInfo* info, const u8* sourcebuffer, u8* destination) {
    console({.str = fmt_("unpack_texture with {} original_bytes, {} compressed_bytes, and {} for dims",
                 info->original_byte_size,
                 info->compressed_byte_size,
                 info->dimensions)});

    if (info->compression_mode == CompressionMode_Lz4) {
        LZ4_decompress_safe((const char*) sourcebuffer, (char*) destination, (s32) info->compressed_byte_size, (s32) info->original_byte_size);
    } else {
        memcpy(destination, sourcebuffer, info->original_byte_size);
    }
}

AssetFile pack_texture(TextureInfo* info, void* pixel_data) {
    console({.str = fmt_("pack_texture with {} bytes and {} for dims", info->original_byte_size, info->dimensions)});
    // core file header
    AssetFile file;
    file.type[0] = 'T';
    file.type[1] = 'E';
    file.type[2] = 'X';
    file.type[3] = 'I';
    file.version = 1;

    bool compress = true;
    if (compress) {
        // Compress binary blob using LZ4
        s32         src_byte_size     = (s32) info->original_byte_size;
        const char* src_data          = (char*) pixel_data;
        s32         max_dst_byte_size = LZ4_compressBound(src_byte_size);
        file.binary_blob.resize(max_dst_byte_size);
        auto dst_data = (char*) file.binary_blob.data();

        int   compressed_byte_size = LZ4_compress_default(src_data, dst_data, src_byte_size, max_dst_byte_size);
        float compression_rate     = float(compressed_byte_size) / float(info->original_byte_size);

        // if the compression is more than 80% of the original size, its not worth to use it
        if (compression_rate > 0.8f) {
            info->compression_mode = CompressionMode_None;
            compressed_byte_size   = src_byte_size;
            file.binary_blob.resize(compressed_byte_size);
            memcpy(file.binary_blob.data(), pixel_data, compressed_byte_size);
        } else {
            info->compression_mode = CompressionMode_Lz4;
            file.binary_blob.resize(compressed_byte_size);
        }
        info->compressed_byte_size = compressed_byte_size;
        // Finished compression state
    } else {
        info->compression_mode = CompressionMode_None;
        file.binary_blob.resize(info->original_byte_size);
        memcpy(file.binary_blob.data(), pixel_data, info->original_byte_size);
        info->compressed_byte_size = info->original_byte_size;
    }

    file.json = ((json_value) *info).dump();

    return file;
}

}
