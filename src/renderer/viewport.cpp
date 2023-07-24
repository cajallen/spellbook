#include "viewport.hpp"

#include <imgui.h>
#include <GLFW/glfw3.h>

#include "extension/fmt.hpp"
#include "extension/fmt_geometry.hpp"
#include "general/math/matrix_math.hpp"
#include "game/input.hpp"

namespace spellbook {

bool viewport_click(ClickCallbackArgs args) {
	Viewport& viewport = *((Viewport*) args.data);
	if (args.action == GLFW_PRESS)
		viewport.focused = viewport.hovered;
	return false;
}

bool viewport_cursor(CursorCallbackArgs args) {
	Viewport& viewport = *((Viewport*) args.data);

    if (Input::cursor_disabled)
        return false;

	viewport.hovered = viewport.window_hovered && math::contains(range2((v2) viewport.start, (v2) (viewport.start + viewport.size)), Input::mouse_pos);
	return false;
}

bool Viewport::update_size(v2i new_size) {
	if (size != new_size) {
		size	   = new_size;
		size_dirty = true;
	}
    return size_dirty;
}
void Viewport::pre_render() {
	if (size_dirty) {
		camera->set_aspect_xy(aspect_xy());
		size_dirty = false;
	}
	camera->pre_render();
}
float Viewport::aspect_xy() {
	return size.y == 0.0f ? 1.0f : size.x / (float) size.y;
}

void Viewport::setup() {
	Input::add_callback(InputCallbackInfo{viewport_click, 0, name, this});
	Input::add_callback(InputCallbackInfo{viewport_cursor, 0, name, this});
}

void inspect(Viewport* viewport) {
	ImGui::Text(fmt_("Start: {}", viewport->start).c_str());
	ImGui::Text(fmt_("Size:  {}", viewport->size).c_str());
    ImGui::Text(fmt_("Mouse Status: Hovered={}, Focused={}", viewport->hovered ? "T" : "F", viewport->focused ? "T" : "F").c_str());
}

ray3 Viewport::ray(v2i screen_pos) {
    return math::transformed_ray(camera->vp, v2(screen_pos - start) / v2(size));
}

v2 Viewport::mouse_uv() {
    return (Input::mouse_pos - v2(start)) / v2(size);
}

}
