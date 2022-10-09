#pragma once

#include <utility>

#include "string.hpp"
#include "vector.hpp"

#include "lib_ext/glfw.hpp"

#include "umap.hpp"
#include "console.hpp"

using std::tuple;

namespace spellbook {

typedef bool (*key_callback)(GLFWwindow*, int /*key*/, int /*scancode*/, int /*action*/, int /*mods*/, void*);
typedef bool (*mouse_pos_callback)(GLFWwindow*, double /*xpos*/, double /*ypos*/, void*);
typedef bool (*mouse_button_callback)(GLFWwindow*, int /*button*/, int /*action*/, int /*mods*/, void*);
typedef bool (*scroll_callback)(GLFWwindow*, double /*xoffset*/, double /*yoffset*/, void*);
typedef bool (*drop_callback)(GLFWwindow*, int /*file_count*/, const char** /*file_name_list*/, void*);
typedef bool (*framebuffer_size_callback)(GLFWwindow*, int /*xsize*/, int /*ysize*/, void*);

struct Input {
    static void setup();
    static void update();

    static vector<tuple<key_callback, string, void*>>              key_callback_stack;
    static vector<tuple<mouse_pos_callback, string, void*>>        mouse_pos_callback_stack;
    static vector<tuple<mouse_button_callback, string, void*>>     mouse_button_callback_stack;
    static vector<tuple<scroll_callback, string, void*>>           scroll_callback_stack;
    static vector<tuple<drop_callback, string, void*>>             drop_callback_stack;             // not implemented
    static vector<tuple<framebuffer_size_callback, string, void*>> framebuffer_size_callback_stack; // not implemented

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
};

void default_key_callback(GLFWwindow*, int, int, int, int);
void default_mouse_pos_callback(GLFWwindow*, double, double);
void default_mouse_button_callback(GLFWwindow*, int, int, int);
void default_scroll_callback(GLFWwindow*, double, double);
void default_drop_callback(GLFWwindow*, int, const char**);
void default_framebuffer_size_callback(GLFWwindow*, int, int);

}
