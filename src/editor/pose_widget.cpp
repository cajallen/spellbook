#include "pose_widget.hpp"

#include "widget_system.hpp"
#include "game/game.hpp"
#include "game/input.hpp"
#include "general/logger.hpp"
#include "general/math/matrix_math.hpp"
#include "renderer/draw_functions.hpp"
#include "renderer/render_scene.hpp"
#include "renderer/viewport.hpp"

namespace spellbook {

// generates lines and handles button state
void _translation_widget(const WidgetSystem::Mouse3DInfo& mouse, PoseWidgetState* state, const PoseWidgetSettings& settings) {
    auto& viewport = settings.render_scene.viewport;

    array<bool, 3> flip_axis;
    flip_axis[0] = -mouse.os_center_ray.dir[0] < 0.0f;
    flip_axis[1] = -mouse.os_center_ray.dir[1] < 0.0f;
    flip_axis[2] = -mouse.os_center_ray.dir[2] < 0.0f;

    // Lines
    for (int axis = 0; axis < 3; axis++) {
        uint32 operation_index = Operation_TranslateX + axis;
        if (!(state->enabled & (0b1 << operation_index)))
            continue;
        
        auto info = mouse_to_3d_line(mouse, settings.transform1d_size * flip_axis[axis] ? -1.0f : 1.0f, axis);
        state->value[operation_index] = info.axis_value;
        if (info.distance < settings.selection_radius) {
            state->hovered |= 0b1 << operation_index;
            state->depth[operation_index] = math::distance(info.position, viewport.camera->position);
        }

        vector<FormattedVertex> vertices;
        v3 pos = v3(0.0f);
        pos[axis] = settings.transform1d_size * (flip_axis[axis] ? -1.0f : 1.0f);
        Color color = Color(v3(settings.color_neutral));
        if ((state->pressed | state->hovered) & 0b1 << operation_index)
            color = Color(v3(settings.color_neutral_hovered));
        color.data[axis] = settings.color_axis;

        float fade = 0.02f;
        if (!flip_axis[axis]) {
            vertices.emplace_back(math::apply_transform(state->model, v3(0.0f)), color, settings.line_radius);
            vertices.emplace_back(math::apply_transform(state->model, pos * (1.0f - fade)), color, settings.line_radius);
            vertices.emplace_back(math::apply_transform(state->model, pos), color, 0.0f);
        } else {
            constexpr int dots = 8;
            constexpr float fdots = float(dots);
            for (int i = 0; i < dots; i++) {
                float f = float(i);
                vertices.emplace_back(math::apply_transform(state->model, pos * (1.0f / fdots * f + 0.0f / fdots)), color, settings.line_radius);
                vertices.emplace_back(math::apply_transform(state->model, pos * (1.0f / fdots * f + 0.5f / fdots)), color, settings.line_radius);
                vertices.emplace_back(math::apply_transform(state->model, pos * (1.0f / fdots * f + 0.5f / fdots + fade)), color, 0.0f);
                vertices.emplace_back(math::apply_transform(state->model, pos * (1.0f / fdots * f + 1.0f / fdots - fade)), color, 0.0f);
            }

        }
    
        auto line_mesh = generate_formatted_line(viewport.camera, std::move(vertices));
        settings.render_scene.quick_mesh(line_mesh, true, settings.widget);
    }

    // Planes
    for (int axis = 0; axis < 3; axis++) {
        int axis1 = (axis + 0) % 3;
        int axis2 = (axis + 1) % 3;
        int axis3 = (axis + 2) % 3;
        uint32 operation_index = Operation_TranslateXY + axis;
        if (!(state->enabled & (0b1 << operation_index)))
            continue;

        auto info = mouse_to_3d_plane(mouse, axis3);
        
        v3 plane_normal = {};
        plane_normal[axis3] = 1.0f;
        if (math::abs(math::dot(plane_normal, mouse.os_center_ray.dir)) < settings.transform2d_dot_cutoff) {
            state->hidden |= 0b1 << operation_index;
            continue;
        }
        
        auto square = range2(v2(settings.transform2d_start), v2(settings.transform2d_start + settings.transform2d_size));
        if (flip_axis[axis1]) {
            auto new_start_axis1 = -1.0f * square.end[0];
            square.end[0] = -1.0f * square.start[0];
            square.start[0] = new_start_axis1;
        }
        if (flip_axis[axis2]) {
            auto new_start_axis2 = -1.0f * square.end[1];
            square.end[1] = -1.0f * square.start[1];
            square.start[1] = new_start_axis2;
        }

        v2 plane_pos(info.position[axis1], info.position[axis2]);
        if (math::contains(square, plane_pos)) {
            state->hovered |= 0b1 << operation_index;
            state->depth[operation_index] = math::distance(info.position, viewport.camera->position);
        }

        v3 p1 = {}, p2 = {}, p3 = {}, p4 = {};
        p1[axis1] = square.start[0];
        p1[axis2] = square.start[1];
        p2[axis1] = square.start[0];
        p2[axis2] = square.end[1];
        p3[axis1] = square.end[0];
        p3[axis2] = square.end[1];
        p4[axis1] = square.end[0];
        p4[axis2] = square.start[1];

        Color color = Color(v3(settings.color_neutral));
        if ((state->pressed | state->hovered) & 0b1 << operation_index)
            color = Color(v3(settings.color_neutral_hovered));
        color.data[axis1] = color.data[axis2] = settings.color_axis;

        vector<FormattedVertex> vertices;
        vertices.emplace_back(math::apply_transform(state->model, p1), color, settings.line_radius);
        vertices.emplace_back(math::apply_transform(state->model, p2), color, settings.line_radius);
        vertices.emplace_back(math::apply_transform(state->model, p3), color, settings.line_radius);
        vertices.emplace_back(math::apply_transform(state->model, p4), color, settings.line_radius);
        vertices.emplace_back(math::apply_transform(state->model, p1), color, settings.line_radius);
        auto line_mesh = generate_formatted_line(viewport.camera, std::move(vertices));
        settings.render_scene.quick_mesh(line_mesh, true, settings.widget);
    }
}
void _rotation_widget(const WidgetSystem::Mouse3DInfo& mouse, PoseWidgetState* state, const PoseWidgetSettings& settings) {
    auto& viewport = settings.render_scene.viewport;

    float dot_aligned_warning = 1.0f;
    for (int axis = 0; axis < 3; axis++) {
        uint32 operation_index = Operation_RotateX + axis;
        if (!(state->enabled & (0b1 << operation_index)))
            continue;

        v3 axis_dir = {};
        axis_dir[axis] = 1.0f;
        float dot = math::dot(axis_dir, mouse.os_center_ray.dir);
        float abs_dot = math::abs(dot);
        
        float percentage = math::map_range(abs_dot, settings.rotation_flatstep_dot, settings.rotation_flatstep_percentage);
        v2 start_vec = {mouse.os_center_ray.dir[(axis + 1) % 3], mouse.os_center_ray.dir[(axis + 2) % 3]};
        float start_angle = math::atan2(start_vec.y, start_vec.x) + math::PI;
        auto angle_range = range(start_angle - percentage * math::PI, start_angle + percentage * math::PI);
        
        auto info = mouse_to_3d_circle(mouse, settings.rotation_size, axis, angle_range);
        state->value[operation_index] = info.angle;
        if (abs_dot < settings.rotation_dot_cutoff) {
            state->hidden |= 0b1 << operation_index;
            continue;
        }
        dot_aligned_warning = math::min(dot_aligned_warning, math::smoothstep(settings.rotation_dot_aligned + 0.01f, settings.rotation_dot_aligned - settings.cutoff_warning, abs_dot));
        
        if (info.distance < settings.selection_radius) {
            state->hovered |= 0b1 << operation_index;
            state->depth[operation_index] = math::distance(info.position, viewport.camera->position);
        }

        vector<FormattedVertex> vertices;
        for (int i = 0; i <= 48; i++) {
            // We do want to interpolate the long way here
            float angle = math::lerp(float(i) / 48.0f, angle_range);
            v3 pos = v3(0.0f);
            pos[(axis + 1) % 3] = settings.rotation_size * math::cos(angle);
            pos[(axis + 2) % 3] = settings.rotation_size * math::sin(angle);
            Color color = Color(v3(settings.color_neutral));
            if ((state->pressed | state->hovered) & 0b1 << operation_index)
                color = Color(v3(settings.color_neutral_hovered));
            color.data[axis] = settings.color_axis;
            
            float dot_cutoff_warning = math::smoothstep(settings.rotation_dot_cutoff - 0.01f, settings.rotation_dot_cutoff + settings.cutoff_warning, abs_dot);
            vertices.emplace_back(math::apply_transform(state->model, pos), color, i == 0 || i == 48 ? 0.0f : dot_cutoff_warning * settings.line_radius);
        }
    
        auto line_mesh = generate_formatted_line(viewport.camera, std::move(vertices));
        settings.render_scene.quick_mesh(line_mesh, true, settings.widget);
    }

    if (state->enabled & (0b1 << Operation_RotateCamera) && dot_aligned_warning > 0.0f) {
        vector<FormattedVertex> vertices;
        for (int i = 0; i <= 48; i++) {
            float angle = i * math::TAU / 48.0f;
            v3 pos = v3(0.0f);
            v3 normal = math::normalize(math::cross(mouse.os_center_ray.dir, v3::Z));
            v3 tangent = math::normalize(math::cross(normal, mouse.os_center_ray.dir));
            pos += settings.rotation_size * math::cos(angle) * normal;
            pos += settings.rotation_size * math::sin(angle) * tangent;
            Color color = Color(v3(settings.color_axis));
            vertices.emplace_back(math::apply_transform(state->model, pos), color, dot_aligned_warning * settings.line_radius);
        }
        
        auto line_mesh = generate_formatted_line(viewport.camera, std::move(vertices));
        settings.render_scene.quick_mesh(line_mesh, true, settings.widget);
    }
}

void _pose_widget(PoseWidgetState* state, const PoseWidgetSettings& settings) {
    auto& viewport = settings.render_scene.viewport;
    
    WidgetSystem::Mouse3DInfo mouse;
    mouse.model = state->model;
    mouse.mvp = viewport.camera->vp * state->model;
    mouse.uv_position = viewport.mouse_uv();
    mouse.os_ray = math::transformed_ray(mouse.mvp, mouse.uv_position);
    v4 h_screen_position = mouse.mvp * v4(v3(0.0f), 1.0f);
    v2 uv_screen_position = math::to_unsigned_range(h_screen_position.xy / h_screen_position.w);
    mouse.os_center_ray = math::transformed_ray(mouse.mvp, uv_screen_position);
    mouse.viewport_size = v2(viewport.size);
    
    state->hovered = 0u;
    
    _translation_widget(mouse, state, settings);
    _rotation_widget(mouse, state, settings);
}

void disable_invalid_operations(v3* position, quat* rotation, PoseWidgetState& state, const PoseWidgetSettings& settings) {
    if (!position) {
        state.enabled &= ~(0b1 << Operation_TranslateX | 0b1 << Operation_TranslateY | 0b1 << Operation_TranslateZ);
    }
    if (!rotation) {
        state.enabled &= ~(0b1 << Operation_RotateX | 0b1 << Operation_RotateY | 0b1 << Operation_RotateZ);
    }
    state.enabled &= ~(0b1 << Operation_TranslateXY | 0b1 << Operation_TranslateYZ | 0b1 << Operation_TranslateZX);
    state.enabled &= ~(0b1 << Operation_RotateCamera | 0b1 << Operation_TranslateCamera);
    state.enabled &= ~(settings.disabled);
}

bool pose_widget(uint64 id, v3* position, quat* rotation, const PoseWidgetSettings& settings, m44* model, PoseWidgetState* out_state) {
    static umap<uint32, PoseWidgetState> widget_states;
    if (widget_states.count(id) == 0)
        widget_states.insert({id, PoseWidgetState{}});
    auto& state = widget_states[id];

    disable_invalid_operations(position, rotation, state, settings);
    
    v3 loc = (position ? *position : v3());
    quat rot = (rotation ? *rotation : quat());

    if (model != nullptr)
        state.model = *model * math::translate(loc) * math::rotation(rot);
    else
        state.model = math::translate(loc) * math::rotation(rot);

    _pose_widget(&state, settings);

    uint32 closest_index = 0;
    float closest_depth = FLT_MAX;
    for (uint32 i = 0; i < Operation_Count; i++) {
        if (state.hovered & 0b1 << i) {
            if (state.depth[i] < closest_depth) {
                closest_index = i;
                closest_depth = state.depth[i];
            }
        }
    }

    WidgetSystem::depths[id] = math::min(closest_depth, WidgetSystem::depths[id]);

    if (WidgetSystem::pressed_id == id) {
        // First clicked frame
        if (!state.pressed) {
            // If this is the last frame, we need to discard
            if (closest_depth < FLT_MAX) {
                state.pressed = 0b1 << closest_index;
                for (uint32 i = 0; i < Operation_Count; i++)
                    state.clicked_value[i] = state.value[i];
                    
                if (position)
                    state.clicked_position = *position;
                if (rotation)
                    state.clicked_rotation = *rotation;
            } else {
                WidgetSystem::pressed_id = 0;
            }
        }
        // Apply the operation
        else {
            v4 rotation_axis_amount = {};
            v3 translation_operation = {};
            for (int i = 0; i < 3; i++) {
                if (position) {
                    int operation_index = Operation_TranslateX + i;
                    if (state.pressed & (0b1 << (operation_index))) {
                        translation_operation[i] = (state.clicked_value[operation_index] - state.value[operation_index]);
                    }
                }
                if (rotation) {
                    int operation_index = Operation_RotateX + i;
                    if (state.pressed & (0b1 << (operation_index))) {
                        rotation_axis_amount[i] = 1.0f;
                        rotation_axis_amount[3] = state.clicked_value[operation_index] - state.value[operation_index];
                    }
                }
            }
            if (position && translation_operation != v3()) {
                if (rotation)
                    *position = *position - math::rotate(*rotation, translation_operation);
                else
                    *position = *position - translation_operation;
            }
            if (rotation && rotation_axis_amount != v4()) {
                *rotation = math::rotate(*rotation, quat(rotation_axis_amount.xyz, -rotation_axis_amount.w));
            } 
        }
    }
    else {
        // Just unclicked
        if (state.pressed) {
            state.pressed = 0;
            for (int i = 0; i < Operation_Count; i++)
                state.clicked_value[i] = 0.0f;
            state.clicked_position = v3();
            state.clicked_rotation = quat();
        }
    }

    if (out_state != nullptr) {
        *out_state = state;
    }
    return state.pressed;
}

void inspect(PoseWidgetState* state) {
    auto flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter;
    if (ImGui::BeginTable("WidgetState", 7, flags)) {
        ImGui::TableSetupColumn("Operation", ImGuiTableColumnFlags_WidthFixed, 190.0f);
        ImGui::TableSetupColumn("Enabled", ImGuiTableColumnFlags_WidthFixed, 55.0f);
        ImGui::TableSetupColumn("Hovered", ImGuiTableColumnFlags_WidthFixed, 55.0f);
        ImGui::TableSetupColumn("Pressed", ImGuiTableColumnFlags_WidthFixed, 55.0f);
        ImGui::TableSetupColumn("Depth", ImGuiTableColumnFlags_WidthFixed, 55.f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 55.f);
        ImGui::TableSetupColumn("Clicked", ImGuiTableColumnFlags_WidthFixed, 55.f);
        ImGui::TableHeadersRow();
        
        for (int row = 0; row < Operation_Count; row++) {
            ImGui::PushID(row);
            ImGui::TableNextRow();
            
            ImGui::TableSetColumnIndex(0);
            auto name = string(magic_enum::enum_name((Operation) row));
            ImGui::Text("%s", name.c_str());
            
            ImGui::TableSetColumnIndex(1);
            ImGui::CheckboxFlags("##Enabled", &state->enabled, 0b1 << row);
            
            ImGui::TableSetColumnIndex(2);
            ImGui::CheckboxFlags("##Hovered", &state->hovered, 0b1 << row);
            
            ImGui::TableSetColumnIndex(3);
            ImGui::CheckboxFlags("##Pressed", &state->pressed, 0b1 << row);
            
            ImGui::TableSetColumnIndex(4);
            if (state->hovered & 0b1 << row)
                ImGui::Text("%.3f", state->depth[row]);
            else
                ImGui::Text("-");

            ImGui::TableSetColumnIndex(5);
            ImGui::Text("%.3f", state->value[row]);

            ImGui::TableSetColumnIndex(6);
            if (state->pressed & 0b1 << row)
                ImGui::Text("%.3f", state->clicked_value[row]);
            else
                ImGui::Text("-");
            
            ImGui::PopID();
        }
        ImGui::EndTable();
    }    
}

}
