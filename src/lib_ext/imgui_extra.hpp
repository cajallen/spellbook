#pragma once

#include <filesystem>
#include <functional>

#include "matrix.hpp"
#include "string.hpp"

#include "geometry.hpp"

#include "renderer/assets/asset_loader.hpp"

namespace fs = std::filesystem;

bool DragMat3(const string& name, spellbook::m33* matrix, f32 speed, const string& format);
bool DragMat4(const string& name, spellbook::m44* matrix, f32 speed, const string& format);

// Shows a selectable of path
void InspectFile(const fs::path& path, fs::path* p_selected);

// Shows a selectable treenode of path, recurses directory contents
void InspectDirectory(const fs::path& path, fs::path* p_selected, const std::function<bool(const fs::path&)>& filter, bool open_subdirectories = true);

// Enables the last item as a drag and drop source
void PathSource(const fs::path& in_path, string dnd_key = "");

// Enables the last item as a drag and drop target
void PathTarget(fs::path* out, const string& dnd_key);

// A widget to select paths
void PathSelect(const string& hint, fs::path* out, const fs::path& base_folder, const std::function<bool(const fs::path&)>& filter, const string& dnd_key, bool open_subdirectories = true);

void PathSelectBody(fs::path* out, const fs::path& base_folder, const std::function<bool(const fs::path&)>& filter, bool* p_open = nullptr, bool open_subdirectories = true);