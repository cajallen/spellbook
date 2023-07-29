#pragma once

#include "general/file_path.hpp"

namespace spellbook {

FilePath resource_path(const string& val);

FilePath operator""_rp(const char* str, uint64 length);

}