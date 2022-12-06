#include "input.hpp"

#include <imgui/backends/imgui_impl_glfw.h>
#include <tracy/Tracy.hpp>

#include "extension/fmt.hpp"
#include "extension/fmt_geometry.hpp"
#include "general/logger.hpp"
#include "game/game.hpp"

namespace spellbook {

vector<tuple<key_callback, string, void*>>              Input::key_callback_stack;
vector<tuple<mouse_pos_callback, string, void*>>        Input::mouse_pos_callback_stack;
vector<tuple<mouse_button_callback, string, void*>>     Input::mouse_button_callback_stack;
vector<tuple<scroll_callback, string, void*>>           Input::scroll_callback_stack;
vector<tuple<drop_callback, string, void*>>             Input::drop_callback_stack;
vector<tuple<framebuffer_size_callback, string, void*>> Input::framebuffer_size_callback_stack;

GLFWwindow*     Input::window;
float           Input::time;
float           Input::delta_time;
v2i             Input::window_pos;
v2              Input::mouse_pos;
v2              Input::mouse_delta;
bool            Input::mouse_click[5];
float           Input::mouse_down[5];
bool            Input::mouse_release[5];
float           Input::mouse_wheel;
bool            Input::ctrl;
bool            Input::shift;
bool            Input::alt;
float           Input::mouse_press_at[5];
bool            Input::exit_requested       = false;
bool            Input::exit_accepted        = false;
bool            Input::cursor_disabled      = false;
bool            Input::cursor_just_disabled = false;
umap<u32, bool> Input::key_down;

void Input::setup() {
    assert_else(game.renderer.window != nullptr)
        return;
    window = game.renderer.window;
    glfwSetKeyCallback(window, default_key_callback);
    glfwSetCursorPosCallback(window, default_mouse_pos_callback);
    glfwSetMouseButtonCallback(window, default_mouse_button_callback);
    glfwSetScrollCallback(window, default_scroll_callback);
    glfwSetDropCallback(window, default_drop_callback);
    glfwSetFramebufferSizeCallback(window, default_framebuffer_size_callback);
    ImGui_ImplGlfw_InstallCallbacks(window);
}

void Input::update() {
    ZoneScoped;
    mouse_delta = v2(0, 0);
    mouse_wheel = 0;
    for (int i = 0; i < 5; i++) {
        mouse_click[i]   = false;
        mouse_release[i] = false;
    }
    glfwPollEvents();

    glfwGetWindowPos(window, &window_pos.x, &window_pos.y);
    float new_time = glfwGetTime();
    delta_time     = new_time - time;
    time           = new_time;
    if (glfwWindowShouldClose(window))
        exit_requested = true;
    if (exit_requested)
        exit_accepted = true;

    // console({.str=format("Mouse Pos: {:}", Input::mouse_pos), .group="Input", .color=palette::white});
}

void default_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    switch (key) {
        case (GLFW_KEY_LEFT_SHIFT): {
            Input::shift = action != GLFW_RELEASE;
        }
        break;
        case (GLFW_KEY_LEFT_ALT): {
            Input::alt = action != GLFW_RELEASE;
        }
        break;
        case (GLFW_KEY_LEFT_CONTROL): {
            Input::ctrl = action != GLFW_RELEASE;
        }
        break;
    }

    Input::key_down[key] = action != GLFW_RELEASE;

    if (ImGui::GetIO().WantCaptureKeyboard)
        return;

    for (auto ptr = Input::key_callback_stack.end() - 1; ptr >= Input::key_callback_stack.begin(); ptr--) {
        auto& [fp, name, data] = *ptr;
        bool  esc              = (*fp)(window, key, scancode, action, mods, data);
        if (esc)
            return;
    }
}

void default_mouse_pos_callback(GLFWwindow* window, double x, double y) {
    Input::mouse_delta = v2(x, y) - Input::mouse_pos;
    if (Input::cursor_just_disabled)
        Input::mouse_delta = v2(0);
    Input::mouse_pos            = v2(x, y);
    Input::cursor_just_disabled = false;

    for (auto ptr = Input::mouse_pos_callback_stack.end() - 1; ptr >= Input::mouse_pos_callback_stack.begin(); ptr--) {
        auto& [fp, name, data] = *ptr;
        bool  esc              = (*fp)(window, x, y, data);
        if (esc)
            return;
    }
}

void default_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (action == GLFW_PRESS) {
        Input::mouse_click[button]    = true;
        Input::mouse_press_at[button] = Input::time;
    }
    if (action == GLFW_REPEAT) {
        Input::mouse_down[button] = Input::time - Input::mouse_press_at[button];
    }
    if (action == GLFW_RELEASE) {
        Input::mouse_release[button]  = true;
        Input::mouse_down[button]     = 0.0f;
        Input::mouse_press_at[button] = -FLT_MAX;
    }

    for (auto ptr = Input::mouse_button_callback_stack.end() - 1; ptr >= Input::mouse_button_callback_stack.begin(); ptr--) {
        auto& [fp, name, data] = *ptr;
        bool  esc              = (*fp)(window, button, action, mods, data);
        if (esc)
            return;
    }
}

void default_scroll_callback(GLFWwindow* window, double x, double y) {
    Input::mouse_wheel = y;

    for (auto ptr = Input::scroll_callback_stack.end() - 1; ptr >= Input::scroll_callback_stack.begin(); ptr--) {
        auto& [fp, name, data] = *ptr;
        bool  esc              = (*fp)(window, x, y, data);
        if (esc)
            return;
    }
}

void default_drop_callback(GLFWwindow*, int, const char**) {
}

void default_framebuffer_size_callback(GLFWwindow*, int x, int y) {
    game.renderer.resize({x, y});
    // game.step(true);
}

void Input::debug_window(bool* p_open) {
    ZoneScoped;
    if (ImGui::Begin("Input", p_open)) {
        ImGui::Text(fmt_("Time={:.1f}, Time Delta={:.3f}", time, delta_time).c_str());
        ImGui::Text(fmt_("Window Pos={}", window_pos).c_str());
        ImGui::Text(fmt_("Mouse Pos={:.0f}, Mouse Delta={:.0f}", mouse_pos, mouse_delta).c_str());
        ImGui::Text(fmt_("Mouse Wheel={:.0f}", mouse_wheel).c_str());
        ImGui::Text(fmt_("Exit Requested={}", exit_requested ? "T" : "F").c_str());
        ImGui::Text(fmt_("Exit Accepted={}", exit_accepted ? "T" : "F").c_str());
        if (ImGui::TreeNode("Key Callbacks")) {
            for (auto [callback, name, data] : key_callback_stack)
                ImGui::Text(name.c_str());
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Mouse Pos Callbacks")) {
            for (auto [callback, name, data] : mouse_pos_callback_stack)
                ImGui::Text(name.c_str());
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Mouse Button Callbacks")) {
            for (auto [callback, name, data] : mouse_button_callback_stack)
                ImGui::Text(name.c_str());
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Pressed Keys")) {
            ImGui::Spacing();
            for (auto [key, state] : key_down) {
                if (state) {
                    ImGui::SameLine();
                    ImGui::Text("%s", glfwGetKeyName(key, 0));
                }
            }
            ImGui::TreePop();
        }
    }
    ImGui::End();
}

void Input::set_cursor_disabled(bool state) {
    cursor_disabled = state;
    if (state) {
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        cursor_just_disabled = true;
    } else {
        ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

void Input::remove_key_callback(const string& name) {
    key_callback_stack.remove_if([&name](const tuple<key_callback, string, void*>& item) { return name == std::get<1>(item); });
}
void Input::remove_mouse_pos_callback(const string& name) {
    mouse_pos_callback_stack.remove_if([&name](const tuple<mouse_pos_callback, string, void*>& item) { return name == std::get<1>(item); });

}
void Input::remove_mouse_button_callback(const string& name) {
    mouse_button_callback_stack.remove_if([&name](const tuple<mouse_button_callback, string, void*>& item) { return name == std::get<1>(item); });
}
void Input::remove_scroll_callback(const string& name) {
    scroll_callback_stack.remove_if([&name](const tuple<scroll_callback, string, void*>& item) { return name == std::get<1>(item); });
}

}
