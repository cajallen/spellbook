#pragma once

#include <functional>
#include <filesystem>

#include "general/logger.hpp"
#include "general/string.hpp"
#include "game/game.hpp"

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

enum FileCategory {
    FileCategory_Json,
    FileCategory_Asset,
    FileCategory_Other
};

FileType file_type_from_path(const fs::path& string);
string extension(FileType type);
std::function<bool(const fs::path&)> path_filter(FileType type);
string dnd_key(FileType type);
FileType from_typeinfo(const type_info& input);
FileCategory file_category(FileType type);

fs::path to_resource_path(const fs::path& path);
fs::path from_resource_path(const fs::path& path);

bool inspect_dependencies(vector<string>& dependencies, const string& current_path);

template <typename T>
umap<string, std::unique_ptr<T>>& asset_cache() {
    static umap<string, std::unique_ptr<T>> t_cache;
    return t_cache;
}

template <typename T>
bool save_asset(const T& asset_value) {
    auto j = from_jv<json>(to_jv(asset_value));
    j["dependencies"] = make_shared<json_value>(to_jv(asset_value.dependencies));
    
    string ext = fs::path(asset_value.file_path).extension().string();
    assert_else(ext == extension(from_typeinfo(typeid(T))))
        return false;
    
    file_dump(j, to_resource_path(asset_value.file_path).string());
    return true;
}

// Usually the ref will want to be copied
template <typename T>
T& load_asset(const string& file_path, bool assert_exists = false, bool clear_cache = false) {
    fs::path absolute_path = to_resource_path(file_path);
    string absolute_path_string = absolute_path.string();
    if (clear_cache && asset_cache<T>().contains(absolute_path_string))
        asset_cache<T>().erase(absolute_path_string);
    else if (asset_cache<T>().contains(absolute_path_string))
        return *asset_cache<T>()[absolute_path_string];
    
    bool exists = fs::exists(absolute_path_string);
    string ext = absolute_path.extension().string();
    bool corrext = ext == extension(from_typeinfo(typeid(T)));
    if (assert_exists) {
        assert_else(exists && corrext)
            return *asset_cache<T>().emplace(absolute_path_string, std::make_unique<T>()).first->second;
    } else {
        check_else(exists && corrext)
            return *asset_cache<T>().emplace(absolute_path_string, std::make_unique<T>()).first->second;
    }

    json& j = game.asset_system.load_json(absolute_path_string);
    
    T& t = *asset_cache<T>().emplace(absolute_path_string, std::make_unique<T>(from_jv<T>(to_jv(j)))).first->second;
    t.dependencies = game.asset_system.load_dependencies(j);
    t.file_path = absolute_path.string();
    return t;
}

}

namespace ImGui {
bool PathSelect(const string& hint, fs::path* out, const fs::path& base_folder, spellbook::FileType type, int open_subdirectories = 1, const std::function<void(const fs::path&)>& context_callback = {});
bool PathSelect(const string& hint, string* out, const string& base_folder, spellbook::FileType, int open_subdirectories = 1, const std::function<void(const fs::path&)>& context_callback = {});
}

