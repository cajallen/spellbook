#pragma once

#include "vector.hpp"
#include "string.hpp"

namespace spellbook {

struct AssetFile {
    char       type[4];
    int        version;
    string     json;
    vector<u8> binary_blob;
};

enum CompressionMode { CompressionMode_None, CompressionMode_Lz4 };

bool            save_binary_file(string_view path, const AssetFile& file);
bool            load_binary_file(string_view path, AssetFile* output_file);
CompressionMode parse_compression(string_view f);

}
