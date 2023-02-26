#include "input.hpp"

#include <imgui/backends/imgui_impl_glfw.h>
#include <tracy/Tracy.hpp>

#include "extension/fmt.hpp"
#include "extension/fmt_geometry.hpp"
#include "general/logger.hpp"
#include "editor/console.hpp"
#include "game/game.hpp"

namespace spellbook {

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
        mouse_down[i]    = mouse_press_at[i] > 0.0f ? Input::time - mouse_press_at[i] : 0.0f;
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

    auto& stack = InputCallbackInfo<KeyCallback>::stack();
    for (auto& callback_info : stack) {
        auto& [fp, prio, name, data] = callback_info;
        bool  esc                    = (*fp)({window, key, scancode, action, mods, data});
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

    auto& stack = InputCallbackInfo<CursorCallback>::stack();
    for (auto& callback_info : stack) {
        auto& [fp, prio, name, data] = callback_info;
        bool  esc                    = (*fp)({window, x, y, data});
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

    auto& stack = InputCallbackInfo<ClickCallback>::stack();
    for (auto& callback_info : stack) {
        auto& [fp, prio, name, data] = callback_info;
        ClickCallbackArgs args = {.window = window, .button = button, .action = action, .mods = mods, .data = data};
        bool  esc                    = (*fp)(args);
        if (esc)
            return;
    }
}

void default_scroll_callback(GLFWwindow* window, double x, double y) {
    Input::mouse_wheel = y;

    auto& stack = InputCallbackInfo<ScrollCallback>::stack();
    for (auto& callback_info : stack) {
        auto& [fp, prio, name, data] = callback_info;
        bool  esc                    = (*fp)({window, x, y, data});
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
            for (auto& [callback, prio, name, data] : InputCallbackInfo<KeyCallback>::stack())
                ImGui::Text(name.c_str());
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Mouse Pos Callbacks")) {
            for (auto& [callback, prio, name, data] : InputCallbackInfo<CursorCallback>::stack())
                ImGui::Text(name.c_str());
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Mouse Button Callbacks")) {
            for (auto& [callback, prio, name, data] : InputCallbackInfo<ClickCallback>::stack())
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
        if (ImGui::TreeNode("Mouse State")) {
            ImGui::Text("mouse_pos: %.2f, %.2f", mouse_pos.x, mouse_pos.y);
            ImGui::Text("mouse_click: %c %c %c %c %c",
                mouse_click[0] ? 't' : 'f',
                mouse_click[1] ? 't' : 'f',
                mouse_click[2] ? 't' : 'f',
                mouse_click[3] ? 't' : 'f',
                mouse_click[4] ? 't' : 'f');
            ImGui::Text("mouse_down: %.1f, %.1f, %.1f, %.1f, %.1f",
                mouse_down[0],
                mouse_down[1],
                mouse_down[2],
                mouse_down[3],
                mouse_down[4]);
            ImGui::Text("mouse_press_at: %.1f, %.1f, %.1f, %.1f, %.1f",
                mouse_press_at[0],
                mouse_press_at[1],
                mouse_press_at[2],
                mouse_press_at[3],
                mouse_press_at[4]);
            ImGui::Text("mouse_release:  %c %c %c %c %c",
                mouse_release[0] ? 't' : 'f',
                mouse_release[1] ? 't' : 'f',
                mouse_release[2] ? 't' : 'f',
                mouse_release[3] ? 't' : 'f',
                mouse_release[4] ? 't' : 'f');
            ImGui::Text("mouse_wheel: %.1f", mouse_wheel);
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

}
