#include "camera.hpp"

#include <imgui.h>

#include "extension/imgui_extra.hpp"
#include "general/math.hpp"
#include "general/matrix.hpp"
#include "general/matrix_math.hpp"

namespace spellbook {

void Camera::set_aspect_xy(float new_aspect_xy) {
	if (new_aspect_xy != aspect_xy) {
		aspect_xy  = new_aspect_xy;
		proj_dirty = true;
	}
}
void Camera::pre_render() {
	bool update_vp = proj_dirty || view_dirty;
	if (proj_dirty)
		update_proj();
	if (view_dirty)
		update_view();
	if (update_vp)
		vp = proj * view;
}

Camera::Camera(v3 pos, euler rot) {
	position = pos;
	heading	 = rot;
}

void Camera::update_proj() {
	// console({.s=fmt_("Updated Proj on {:}", *this), .g="Camera", .c = Color(0.8f, 1.0f), .p = false});
	proj	   = math::infinite_perspective(math::d2r(fov / 2.0f), aspect_xy, clip_planes[0]);
	proj_dirty = false;
}

void Camera::update_view() {
	// console({.s=fmt_("Updated View on {:}", *this), .g="Camera", .c = Color(0.8f, 1.0f), .p = false});
	v3 vec	   = math::euler2vector(heading);
	view	   = math::look(position, vec, v3(0, 0, 1));
	view_dirty = false;
}

void inspect(Camera* camera) {
	euler heading_degrees = camera->heading.r2d();
	camera->view_dirty |= ImGui::DragFloat3("Position", &camera->position.x, 0.025f);
	camera->view_dirty |= ImGui::DragFloat2("Direction", &heading_degrees.yaw, 0.5f);
	camera->heading = heading_degrees.d2r();
	camera->proj_dirty |= ImGui::DragFloat("FOV", &camera->fov, 0.1f);
	camera->proj_dirty |= ImGui::DragFloat2("Clip", &camera->clip_planes.x, 0.01f);

	ImGui::Separator();

	bool vp_override = false;
	vp_override |= ImGui::DragMat4("proj", &camera->proj, 0.01f, "%.2f");
	ImGui::Separator();
	vp_override |= ImGui::DragMat4("view", &camera->view, 0.01f, "%.2f");

	if (vp_override)
		camera->vp = camera->proj * camera->view;
}

}
