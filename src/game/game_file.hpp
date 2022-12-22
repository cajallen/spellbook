#pragma once

#include <functional>
#include <filesystem>

#include "general/logger.hpp"
#include "general/string.hpp"

namespace spellbook {

namespace fs = std::filesystem;

enum FileType {
    FileType_Directory,
    FileType_Unknown,
    FileType_General,
    FileType_Model,
    FileType_ModelAsset,
    FileType_Texture,
    FileType_TextureAsset,
    FileType_Mesh,
    FileType_Material,
    FileType_Map,
    FileType_Lizard,
    FileType_Tile,
    FileType_Enemy,
    FileType_Spawner,
    FileType_Consumer
};

string extension(FileType type);
std::function<bool(const fs::path&)> path_filter(FileType type);
string dnd_key(FileType type);
FileType from_typeinfo(const type_info& input);

fs::path to_resource_path(const fs::path& path);
fs::path from_resource_path(const fs::path& path);

template <typename T>
bool save_asset(const T& asset_value) {
    auto j = from_jv<json>(to_jv(asset_value));
    
    string ext = fs::path(asset_value.file_path).extension().string();
    assert_else(ext == extension(from_typeinfo(typeid(T))))
        return false;
    
    file_dump(j, to_resource_path(asset_value.file_path).string());
    return true;
}

template <typename T>
T load_asset(const string& file_path) {
    fs::path absolute_path = to_resource_path(file_path);
    check_else(fs::exists(absolute_path))
        return {};
    string ext = absolute_path.extension().string();
    assert_else(ext == extension(from_typeinfo(typeid(T))))
        return {};

    json j = parse_file(absolute_path.string());
    auto asset_value = from_jv<T>(to_jv(j));
    asset_value.file_path = absolute_path.string();
    return asset_value;
}

}