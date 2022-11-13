#include "gui.hpp"

#include <filesystem>

#include <imgui.h>
#include <tracy/Tracy.hpp>

#include "input.hpp"
#include "game.hpp"
#include "var_system.hpp"
#include "console.hpp"
#include "file.hpp"

#include "game/asset_browser.hpp"

#include "renderer/camera.hpp"

namespace fs = std::filesystem;

namespace spellbook {

void GUI::setup() {
    fs::path gui_file = fs::path(game.user_folder) / ("gui" + extension(FileType_General));

    if (!fs::exists(gui_file))
        return;
    
    json j = parse_file(gui_file.string());
    FROM_JSON_MEMBER(windows);
    FROM_JSON_MEMBER(item_state);
    FROM_JSON_MEMBER(asset_browser_file);
    if (!fs::exists(fs::path(asset_browser_file))) {
        asset_browser_file = fs::current_path().string();
    }
    
}

void GUI::shutdown() {
    fs::path gui_file = fs::path(game.user_folder) / ("gui" + extension(FileType_General));
    fs::create_directories(gui_file.parent_path());
    
    auto j = json();
    TO_JSON_MEMBER(windows);
    TO_JSON_MEMBER(item_state);
    TO_JSON_MEMBER(asset_browser_file);

    file_dump(j, gui_file.string());
}


bool* GUI::window_open(string window_name) {
    if (windows.count(window_name) == 0) {
        windows[window_name] = {true, false};
    }
    return &windows[window_name].opened;
}

#define SCENE_MENU_ITEM(var, key, enabled)                                        \
    if (ImGui::MenuItem(#var, key, current_scene->type == Scene::var, enabled)) { \
        request_scene(var);                                                       \
    }
void GUI::_main_menu_bar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Windows")) {
            for (auto& [k, v] : windows) {
                // if (v.queried) {
                if (ImGui::MenuItem(k.c_str(), NULL, v.opened))
                    v.opened = !v.opened;
                v.queried = false;
                //}
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void GUI::update() {
    ZoneScoped;

    // main
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::SetNextWindowBgAlpha(0.0f);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    ImGuiID            dockspace_id    = ImGui::GetID("Dockspace");
    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    ImGui::End();

    _main_menu_bar();

    bool* p_open;

    if (*(p_open = window_open("demo")))
        ImGui::ShowDemoWindow(p_open);
    if (*(p_open = window_open("map_editor")))
        game.map_editor.window(p_open);
    if (*(p_open = window_open("asset_editor")))
        game.asset_editor.window(p_open);
    // if (*(p_open = window_open("test_scene")))
    //     game.test_scene.window(p_open);
    if (*(p_open = window_open("var_system")))
        VarSystem::window(p_open);
    if (*(p_open = window_open("console")))
        Console::window(p_open);
    for (auto scene : game.scenes) {
        if (*(p_open = window_open(scene->name + " info")))
            scene->settings_window(p_open);
    }
    for (auto scene : game.scenes) {
        if (*(p_open = window_open(scene->name + " output")))
            scene->output_window(p_open);
    }
    if (*(p_open = window_open("input")))
        Input::debug_window(p_open);
    if (*(p_open = window_open("renderer")))
        game.renderer.debug_window(p_open);
    if (*(p_open = window_open("colors")))
        color_window(p_open);
    if (*(p_open = window_open("asset_browser"))) {
        std::filesystem::path asset_browser_path = asset_browser_file;
        asset_browser("Asset Browser", p_open, &asset_browser_path);
        asset_browser_file = asset_browser_path.string();
    }
}

}

