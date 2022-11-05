#pragma once

#include <filesystem>

#include "vector.hpp"
#include "string.hpp"

#include "console.hpp"
#include "game/asset_browser.hpp"

namespace fs = std::filesystem;

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
    FileType_Tower,
    FileType_Tile,
    FileType_Enemy,
    FileType_Spawner,
    FileType_Consumer
};

string extension(FileType type);
std::function<bool(const fs::path&)> path_filter(FileType type);
string dnd_key(FileType type);
FileType from_typeinfo(const type_info& input);

string get_file(const string& str, bool with_ext = false);
string get_folder(const string& str);
string get_extension(const string& str);
string get_contents(const string& file_name, bool binary = false);
vector<u32> get_contents_u32(const string& file_name, bool binary = true);
bool   file_exists(const string& file_name);

fs::path to_resource_path(const fs::path& path);
fs::path from_resource_path(const fs::path& path);


}
