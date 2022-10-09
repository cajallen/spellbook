#pragma once
#include "vuk/Config.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "geometry.hpp"
#include "string.hpp"

inline GLFWwindow* create_window_glfw(const string& title, spellbook::v2i window_size, bool resize = true) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    if (!resize)
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    return glfwCreateWindow(window_size.x, window_size.y, title.c_str(), NULL, NULL);
}

inline void destroy_window_glfw(GLFWwindow* window) {
    glfwDestroyWindow(window);
    glfwTerminate();
}

inline VkSurfaceKHR create_surface_glfw(VkInstance instance, GLFWwindow* window) {
    VkSurfaceKHR surface = nullptr;
    VkResult     err     = glfwCreateWindowSurface(instance, window, NULL, &surface);
    if (err) {
        const char* error_msg;
        int         ret = glfwGetError(&error_msg);
        if (ret != 0) {
            throw error_msg;
        }
        surface = nullptr;
    }
    return surface;
}