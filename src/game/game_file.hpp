#pragma once

#include <functional>
#include <filesystem>

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

}