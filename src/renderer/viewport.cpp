#include "viewport.hpp"

#include <imgui.h>
#include <GLFW/glfw3.h>

#include "extension/fmt_geometry.hpp"
#include "lib/matrix_math.hpp"
#include "game/input.hpp"

namespace spellbook {

void Viewport::update_size(v2i new_size) {
	if (size != new_size) {
		size	   = new_size;
		size_dirty = true;
	}
}
void Viewport::pre_render() {
	if (size_dirty) {
		camera->set_aspect_xy(aspect_xy());
		size_dirty = false;
	}
	camera->pre_render();
}
f32 Viewport::aspect_xy() {
	return size.y == 0.0f ? 1.0f : size.x / (f32) size.y;
}

bool viewport_mb_callback(GLFWwindow* window, int button, int action, int mods, void* data) {
	Viewport& viewport = *((Viewport*) data);
	if (action == GLFW_PRESS)
		viewport.focused = viewport.hovered;
	return false;
}

bool viewport_mp_callback(GLFWwindow* window, double xpos, double ypos, void* data) {
	Viewport& viewport = *((Viewport*) data);

    if (Input::cursor_disabled) return false;

	viewport.hovered = viewport.window_hovered && math::contains(range2((v2) viewport.start, (v2) (viewport.start + viewport.size)), Input::mouse_pos);
	return false;
}

void Viewport::setup() {
	Input::mouse_button_callback_stack.push_back({viewport_mb_callback, name, this});
	Input::mouse_pos_callback_stack.push_back({viewport_mp_callback, name, this});
}

void inspect(Viewport* viewport) {
	ImGui::Text(fmt_("Start: {}", viewport->start).c_str());
	ImGui::Text(fmt_("Size:  {}", viewport->size).c_str());
    ImGui::Text(fmt_("Mouse Status: Hovered={}, Focused={}", viewport->hovered ? "T" : "F", viewport->focused ? "T" : "F").c_str());
}

r3 Viewport::ray(v2i screen_pos) {
    return math::transformed_ray(camera->vp, v2(screen_pos - start) / v2(size));
}

}
