#pragma once

#include "string.hpp"

#include "umap.hpp"
#include "json.hpp"
#include "color.hpp"


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

struct GUI {
    // u64 should come from id_ptr
    umap<u64, InterfaceInfo> item_state;
    umap<string, WindowState>  windows;

    void setup();
    void _main_menu_bar();
    void update();
    void shutdown();

    bool* window_open(string window_name);
};

JSON_IMPL(InterfaceInfo, opened, color);
JSON_IMPL(WindowState, opened);

}
