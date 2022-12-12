#pragma once

#include <utility>

#include "extension/glfw.hpp"
#include "general/string.hpp"
#include "general/vector.hpp"
#include "general/umap.hpp"

namespace spellbook {

struct KeyCallbackArgs {
    GLFWwindow* window;
    int key, scancode, action, mods;
    void* data;
};

struct CursorCallbackArgs {
    GLFWwindow* window;
    double x, y;
    void* data;
};

struct ClickCallbackArgs {
    GLFWwindow* window;
    int button, action, mods;
    void* data;
};

struct ScrollCallbackArgs {
    GLFWwindow* window;
    double xoffset, yoffset;
    void* data;
};

typedef bool (*KeyCallback)(KeyCallbackArgs);
typedef bool (*CursorCallback)(CursorCallbackArgs);
typedef bool (*ClickCallback)(ClickCallbackArgs);
typedef bool (*ScrollCallback)(ScrollCallbackArgs);
// typedef bool (*drop_callback)(GLFWwindow*, int /*file_count*/, const char** /*file_name_list*/, void*);
// typedef bool (*framebuffer_size_callback)(GLFWwindow*, int /*xsize*/, int /*ysize*/, void*);

template<typename T>
struct InputCallbackInfo {
    T callback;
    u32 priority;
    string name;
    void* data;

    static vector<InputCallbackInfo<T>>& stack() {
        static vector<InputCallbackInfo<T>> s;
        return s;
    }
};

struct Input {
    static void setup();
    static void update();

    static GLFWwindow* window;
    static float       time;
    static float       delta_time;
    // window state
    static bool exit_requested;
    static bool exit_accepted;
    static v2i  window_pos;
    static bool cursor_disabled;
    static bool cursor_just_disabled; // first frame that cursor is disabled has some trouble with mouse position
    // mouse info
    static v2    mouse_pos;
    static v2    mouse_delta;
    static bool  mouse_click[5];
    static float mouse_down[5];
    static float mouse_press_at[5];
    static bool  mouse_release[5];
    static float mouse_wheel;
    // modifiers
    static bool ctrl;
    static bool shift;
    static bool alt;
    // keys
    static umap<u32, bool> key_down;

    static void debug_window(bool* p_open);

    static void set_cursor_disabled(bool state = true);

    template<typename T>
    static void remove_callback(const string& name) {
        auto& stack = InputCallbackInfo<T>::stack();

        stack.remove_if([&name](const InputCallbackInfo<T>& element) { return element.name == name; }, false);
    }

    template<typename T>
    static void add_callback(const InputCallbackInfo<T>& callback_info) {
        auto& stack = InputCallbackInfo<T>::stack();

        for (u32 i = 0; i < stack.size(); i++) {
            if (callback_info.priority < stack[i].priority) {
                stack.insert(i, callback_info);
                return;
            }
        }
        stack.push_back(callback_info);
    }
};

void default_key_callback(GLFWwindow*, int, int, int, int);
void default_mouse_pos_callback(GLFWwindow*, double, double);
void default_mouse_button_callback(GLFWwindow*, int, int, int);
void default_scroll_callback(GLFWwindow*, double, double);
void default_drop_callback(GLFWwindow*, int, const char**);
void default_framebuffer_size_callback(GLFWwindow*, int, int);

}


/*
 * Priorities:
 *
 * Viewport info: 0
 * Painting while brush enabled: 20
 * Widgets: 40
 * Dragging start: 60
 *
 * Map editor hotkeys: 60
 */
