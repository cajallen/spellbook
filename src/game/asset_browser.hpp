#pragma once

#include <filesystem>
#include <functional>

#include "string.hpp"
#include "renderer/assets/asset_loader.hpp"

namespace fs = std::filesystem;

namespace spellbook {


void asset_browser(const string& window_name, fs::path* out);

bool possible_file(const fs::path& path);
bool possible_folder(const fs::path& path);
bool possible_model(const fs::path& path);
bool possible_material(const fs::path& path);
bool possible_texture(const fs::path& path);
bool possible_mesh(const fs::path& path);
bool possible_model_asset(const fs::path& path);
bool possible_mesh_asset(const fs::path& path);
bool possible_texture_asset(const fs::path& path);

}