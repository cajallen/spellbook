#include "skeleton_widget.hpp"

#include <Tracy/Tracy.hpp>

#include "general/matrix_math.hpp"
#include "game/input.hpp"
#include "editor/widget_system.hpp"
#include "editor/console.hpp"
#include "renderer/draw_functions.hpp"
#include "renderer/render_scene.hpp"

namespace spellbook {

void skeleton_widget(SkeletonCPU* skeleton, const m44& model, RenderScene* render_scene) {
    static bool initialized = false;
    static string joint_name;
    if (!initialized) {
        auto joint_mesh = generate_cube(v3(0.0f), v3(0.01f), palette::gray_5);
        joint_name = upload_mesh(joint_mesh, false);
        initialized = true;
    }
    
    vector<FormattedVertex> vertices;
    for (id_ptr<Bone>& bone : skeleton->bones) {
        ZoneScoped;
        m44 bone_transform = model * bone->transform();
        auto& r = render_scene->quick_mesh(joint_name, true, true);
        r.transform = (m44GPU) bone_transform;

        vertices.emplace_back(math::apply_transform(bone_transform, v3(0.0f)), palette::gray_5, 0.003f);
        vertices.emplace_back(math::apply_transform(bone_transform, v3(0.0f, bone->length, 0.0f)), palette::gray_5, 0.003f);
        vertices.emplace_back(FormattedVertex::separate());

        WidgetSystem::Mouse3DInfo mouse;
        mouse.model = bone_transform;
        mouse.mvp = render_scene->viewport.camera->vp * mouse.model;
        mouse.uv_position = render_scene->viewport.mouse_uv();
        mouse.os_ray = math::transformed_ray(mouse.mvp, mouse.uv_position);
        v4 h_screen_position = mouse.mvp * v4(v3(0.0f), 1.0f);
        v2 uv_screen_position = math::to_unsigned_range(h_screen_position.xy / h_screen_position.w);
        mouse.os_center_ray = math::transformed_ray(mouse.mvp, uv_screen_position);
        mouse.viewport_size = v2(render_scene->viewport.size);

        auto line_project_info = mouse_to_3d_line(mouse, bone->length, 1);

        auto& depth = WidgetSystem::depths[bone.id];
        if (line_project_info.distance < 5.0f) {
            constexpr float bias = 0.02f;
            depth = math::min(depth, math::length(line_project_info.position - render_scene->viewport.camera->position) + bias);
        } else {
            depth = math::min(depth, FLT_MAX);
        }
        
        if (Input::mouse_click[0]) {
            if (depth < FLT_MAX) {
                console({"test"});
            } else {
                skeleton->widget_pose_enabled[skeleton->bones.index(bone)] = false;
            }
        }

        auto pressed_id = WidgetSystem::pressed_id;
        if (pressed_id != 0)
            console({"test"});
        if (pressed_id == bone.id)
            skeleton->widget_pose_enabled[skeleton->bones.index(bone)] = true;
    }

    auto line = generate_formatted_line(render_scene->viewport.camera, vertices);
    render_scene->quick_mesh(line, true, true);
}

}