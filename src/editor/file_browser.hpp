#pragma once

#include "general/file/file_path.hpp"

namespace spellbook {

void file_browser(const string& window_name, bool* p_open, FilePath* out);

}