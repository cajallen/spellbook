#pragma once

#include <robin_hood.h>

#include "general/logger.hpp"
#include "general/string.hpp"
#include "game/game.hpp"
#include "general/file_path.hpp"

namespace spellbook {

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

FileType get_type(const FilePath& path);
FileCategory get_category(const FilePath& path);

string extension(FileType type);
string dnd_key(FileType type);
string resource_folder(FileType type); // abs
std::function<bool(const fs::path&)> path_filter(FileType type); // this has to be an fs path because it's fed into imgui_extra
FileType from_typeinfo(const type_info& input);
FileCategory file_category(FileType type);

bool inspect_dependencies(vector<FilePath>& dependencies, const FilePath& current_path);

string get_contents(const FilePath& file, bool binary = false);
vector<uint32> get_contents_uint32(const FilePath& file, bool binary = true);

template <typename T>
umap<FilePath, std::unique_ptr<T>>& cpu_asset_cache() {
    static umap<FilePath, std::unique_ptr<T>> t_cache;
    return t_cache;
}

template <typename T>
bool save_asset(const T& asset_value) {
    auto j = from_jv<json>(to_jv(asset_value));
    j["dependencies"] = make_shared<json_value>(to_jv(asset_value.dependencies));
    
    assert_else(asset_value.file_path.extension() == extension(from_typeinfo(typeid(T))))
        return false;
    
    file_dump(j, asset_value.file_path.abs_string());
    return true;
}

// Usually the ref will want to be copied
template <typename T>
T& load_asset(const FilePath& file_path, bool assert_exists = false, bool clear_cache = false) {
    fs::path absolute_path = file_path.abs_path();
    if (clear_cache && cpu_asset_cache<T>().contains(file_path))
        cpu_asset_cache<T>().erase(file_path);
    else if (cpu_asset_cache<T>().contains(file_path))
        return *cpu_asset_cache<T>()[file_path];
    
    bool exists = fs::exists(absolute_path);
    string ext = absolute_path.extension().string();
    bool corrext = ext == extension(from_typeinfo(typeid(T)));
    if (assert_exists) {
        assert_else(exists && corrext)
            return *cpu_asset_cache<T>().emplace(file_path, std::make_unique<T>()).first->second;
    } else {
        check_else(exists && corrext)
            return *cpu_asset_cache<T>().emplace(file_path, std::make_unique<T>()).first->second;
    }

    json& j = game.asset_system.load_json(file_path);
    
    T& t = *cpu_asset_cache<T>().emplace(file_path, std::make_unique<T>(from_jv<T>(to_jv(j)))).first->second;
    t.dependencies = game.asset_system.load_dependencies(j);
    t.file_path = file_path;
    return t;
}


}

namespace ImGui {

bool PathSelect(const string& hint, spellbook::FilePath* out, const spellbook::FilePath& base_folder, spellbook::FileType type, int open_subdirectories = 1, const std::function<void(const fs::path&)>& context_callback = {});
bool PathSelect(const string& hint, spellbook::FilePath* out, spellbook::FileType type, int open_subdirectories = 1, const std::function<void(const fs::path&)>& context_callback = {});

}

