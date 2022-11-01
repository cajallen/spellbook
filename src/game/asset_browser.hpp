﻿#pragma once

#include <filesystem>

#include "string.hpp"

namespace fs = std::filesystem;

namespace spellbook {

void asset_browser(const string& window_name, bool* p_open, fs::path* out);

}