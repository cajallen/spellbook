#pragma once

#include "string.hpp"
#include "geometry.hpp"

#include "renderer/camera.hpp"
#include "renderer/viewport.hpp"

namespace spellbook {

struct CameraController {
    string    name;
    Viewport* viewport = nullptr; // Controller needs a viewport, because input is theoretically passed through the viewport.
    Camera*   camera   = nullptr;

    enum NavigationMode { NavigationMode_None, NavigationMode_Fly, NavigationMode_Pivot };
    NavigationMode nav_mode = NavigationMode_None;

    struct FlyState {
        int   move_state  = 0;
        float speed       = 4.0f;
        float sensitivity = 0.2f;
    } fly_state;
    struct PivotState {
        float sensitivity = 0.2f;

        bool panning         = false;
        v2   pan_start_mouse = v2(0.f);
        v3   pan_start_world = v3(0.f);

        bool  rotating             = false;
        v2    rotate_start_mouse   = v2(0.f);
        euler rotate_start_heading = euler();

        v3    arm_pivot   = v3(0.f);
        euler arm_heading = euler();
        f32   arm_length  = -1.f;

    } pivot_state;

    void change_state(NavigationMode new_mode);
    void setup(Viewport* viewport, Camera* camera);
    void update();
};

void inspect(CameraController* controller);

bool cc_on_mouse_press(GLFWwindow* window, int button, int action, int mods, void* data);
bool cc_on_cursor_move(GLFWwindow* window, double xpos, double ypos, void* data);
bool cc_on_scroll(GLFWwindow* window, double xpos, double ypos, void* data);
bool cc_on_key(GLFWwindow* window, int key, int scancode, int action, int mods, void* data);

}

