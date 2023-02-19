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
    FileType_Consumer,
    FileType_Emitter,
    FileType_VisualTileSet,
    FileType_Drop,
    FileType_Skeleton
};

string extension(FileType type);
std::function<bool(const fs::path&)> path_filter(FileType type);
string dnd_key(FileType type);
FileType from_typeinfo(const type_info& input);

fs::path to_resource_path(const fs::path& path);
fs::path from_resource_path(const fs::path& path);

template <typename T>
umap<string, T>& asset_cache() {
    static umap<string, T> t_cache;
    return t_cache;
}

template <typename T>
bool save_asset(const T& asset_value) {
    auto j = from_jv<json>(to_jv(asset_value));
    
    string ext = fs::path(asset_value.file_path).extension().string();
    assert_else(ext == extension(from_typeinfo(typeid(T))))
        return false;
    
    file_dump(j, to_resource_path(asset_value.file_path).string());
    return true;
}

// Usually the ref will want to be copied
template <typename T>
T& load_asset(const string& file_path, bool assert_exists = false) {
    fs::path absolute_path = to_resource_path(file_path);
    string absolute_path_string = absolute_path.string();
    if (asset_cache<T>().contains(absolute_path_string))
        return asset_cache<T>()[absolute_path_string];
    
    bool exists = fs::exists(absolute_path_string);
    string ext = absolute_path.extension().string();
    bool corrext = ext == extension(from_typeinfo(typeid(T)));
    if (assert_exists) {
        assert_else(exists && corrext)
            return asset_cache<T>().emplace(absolute_path_string, T()).first->second;
    } else {
        check_else(exists && corrext)
            return asset_cache<T>().emplace(absolute_path_string, T()).first->second;
    }

    json j = parse_file(absolute_path.string());
    T& t = asset_cache<T>().emplace(absolute_path_string, from_jv<T>(to_jv(j))).first->second;
    t.file_path = absolute_path.string();
    return t;
}

}