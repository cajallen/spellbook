#pragma once

#include <filesystem>

#include "vector.hpp"
#include "string.hpp"

namespace fs = std::filesystem;

namespace spellbook {

string get_file(const string& str, bool with_ext = false);
string get_folder(const string& str);
string get_extension(const string& str);
string get_contents(const string& file_name, bool binary = false);
vector<u32> get_contents_u32(const string& file_name, bool binary = true);
bool   file_exists(const string& file_name);


}
