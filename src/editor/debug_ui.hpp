#pragma once

#include "general/string.hpp"
#include "general/umap.hpp"
#include "general/color.hpp"
#include "general/file/json.hpp"
#include "general/file/file_path.hpp"

namespace spellbook {

struct InterfaceInfo {
    bool  opened;
    bool  request_open;
    bool  request_close;
    Color color;
};

struct WindowState {
    bool opened;
    bool queried;
};

struct DebugUI {
    // uint64 should come from id_ptr
    umap<uint64, InterfaceInfo> item_state;
    umap<string, WindowState>  windows;

    FilePath file_browser_path;
    
    void setup();
    void _main_menu_bar();
    void update();
    void shutdown();

    bool* window_open(string window_name);
};

JSON_IMPL(InterfaceInfo, opened, color);
JSON_IMPL(WindowState, opened);

}
