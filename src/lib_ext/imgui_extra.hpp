#pragma once

#include <filesystem>

#include "matrix.hpp"
#include "string.hpp"

#include "geometry.hpp"

namespace fs = std::filesystem;

bool DragMat3(const string& name, spellbook::m33* matrix, f32 speed, const string& format);
bool DragMat4(const string& name, spellbook::m44* matrix, f32 speed, const string& format);


// TODO: Predicates

// Shows a selectable of path
void InspectFile(const fs::path& path, fs::path* p_selected);

// Shows a selectable treenode of path, recurses directory contents
void InspectDirectory(const fs::path& path, fs::path* p_selected);

// Enables the last item as a drag and drop source
void PathSource(const fs::path& in_path, const string& dnd_key);

// Enables the last item as a drag and drop target
void PathTarget(fs::path* out, const string& dnd_key);

// A widget to select paths
void PathSelect(const string& hint, fs::path* out, const fs::path& base_folder, const string& dnd_key);