#pragma once

#include <array>
#include <filesystem>

#include "json.hpp"
#include "vector.hpp"
#include "string.hpp"

using std::array;
namespace fs = std::filesystem;

namespace spellbook {

struct AssetFile {
    string file_name;

    array<char, 3> type;
    int            version;
    json           asset_json;
    vector<u8>     binary_blob;
};

enum CompressionMode { CompressionMode_None, CompressionMode_Lz4 };

void      save_asset_file(const AssetFile& file);
AssetFile load_asset_file(const string& path);

}
