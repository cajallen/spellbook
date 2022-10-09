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

    InterfaceInfo() = default;
    JSON_IMPL(InterfaceInfo, opened, color);
};

struct WindowState {
    bool opened;
    bool queried;
    WindowState() = default;
    WindowState(bool o, bool q) : opened(o), queried(q) {}
    JSON_IMPL(WindowState, opened);
};

struct GUI {
    umap<void*, InterfaceInfo> item_state;
    umap<string, WindowState>  windows;

    GUI() = default;
    JSON_IMPL(GUI, windows);

    void _main_menu_bar();
    void update();

    bool* window_open(string window_name);
};

}
