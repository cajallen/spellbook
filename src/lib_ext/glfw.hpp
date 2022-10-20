#pragma once

#include "vuk/Config.hpp"
#include <GLFW/glfw3.h>

#include "geometry.hpp"
#include "string.hpp"

GLFWwindow* create_window_glfw(const string& title, spellbook::v2i window_size, bool resize = true);
void destroy_window_glfw(GLFWwindow* window);
VkSurfaceKHR create_surface_glfw(VkInstance instance, GLFWwindow* window);