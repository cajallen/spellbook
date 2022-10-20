#include "gui.hpp"
#include "imgui.h"
#include "renderer/camera.hpp"

#include "game.hpp"
#include "var_system.hpp"
#include "console.hpp"

#include <tracy/Tracy.hpp>

#include "input.hpp"

namespace spellbook {

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
    if (*(p_open = window_open("editor")))
        game.editor.window(p_open);
    if (*(p_open = window_open("var_system")))
        VarSystem::window(p_open);
    if (*(p_open = window_open("console")))
        Console::window(p_open);
    for (auto scene : game.scenes) {
        if (*(p_open = window_open(scene->name + " info")))
            scene->window(p_open);
    }
    if (*(p_open = window_open("input")))
        Input::debug_window(p_open);
    if (*(p_open = window_open("renderer")))
        game.renderer.debug_window(p_open);
}

}

