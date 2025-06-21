#include "node_editor.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include "extension/fmt.hpp"
#include "general/logger.hpp"
#include "general/input.hpp"
#include "general/memory.hpp"

namespace spellbook {

void NodeEditorContext::Begin() {
    screen_offset_delta = v2{};
    create_link_request = {};
    create_node_request = false;
    delete_node_requests.clear();
    delete_link_requests.clear();
    movement_source = invalid_id;

    if (Input::mouse_down[GLFW_MOUSE_BUTTON_MIDDLE]) {
        screen_offset_delta = -Input::mouse_delta * _GetScreenSpaceScale();
        screen_offset += screen_offset_delta;
    }

    if (ImGui::IsWindowHovered() && Input::mouse_click[GLFW_MOUSE_BUTTON_LEFT]) {
        selecting = true;
        select_start = v2(ImGui::GetMousePos());
    }

    if (ImGui::IsWindowHovered() && Input::mouse_click[GLFW_MOUSE_BUTTON_RIGHT]) {
        create_node_request = true;
    }

    if (ImGui::IsWindowHovered() && Input::mouse_wheel != 0.0f) {
        int old_zoom_level = zoom_level;
        if (Input::mouse_wheel < 0.0f)
            zoom_level += 1;
        else
            zoom_level -= 1;

        zoom_level = math::clamp(zoom_level, 0, 2);
        if (old_zoom_level != zoom_level) {
            v2 old_cursor_screenspace = v2(ImGui::GetMousePos());
            v2 old_cursor_worldspace = ScreenToWorld(old_cursor_screenspace, old_zoom_level);
            v2 new_cursor_screenspace = WorldToScreen(old_cursor_worldspace, zoom_level);
            screen_offset_delta = (new_cursor_screenspace - old_cursor_screenspace) * _GetScreenSpaceScale(zoom_level);
            screen_offset += screen_offset_delta;
        }
    }

    if (Input::mouse_release[GLFW_MOUSE_BUTTON_LEFT]) {
        selecting = false;
        if (pin_pressed != invalid_id && hovered_pin != invalid_id)
            create_link_request = {pin_pressed, hovered_pin};
    }

    if (Input::key_down[GLFW_KEY_DELETE]) {
        delete_link_requests = selection.links;
        delete_node_requests = selection.nodes;
    }

    if (selecting) {
        selection.clear();

        range2 selection_box(select_start, v2(ImGui::GetMousePos()));
        for (auto& [id, state] : node_states) {
            v2 screen_pos = WorldToScreen(state.world_pos);
            if (math::aabb_overlaps(range2{screen_pos - state.size * 0.5f, screen_pos + state.size * 0.5f}, selection_box))
                selection.nodes.push_back(id);
        }

        for (auto& [id, state] : link_states) {
            if (math::line_intersects_aabb(range2{state.start, state.end}, selection_box))
                selection.links.push_back(id);
        }
    }

    hovered_pin = invalid_id;

    _DrawBackground();
}

void NodeEditorContext::End() {
    if (ImGui::Begin("Debug Output")) {
        ImGui::Text("pin_pressed: %llu", pin_pressed);
        ImGui::Text("hovered_pin: %llu", hovered_pin);
        ImGui::Text("active_node: %llu", active_node);
        ImGui::Text("active_pin: %llu", active_pin);
        ImGui::Text("create_link_request: %llu, %llu", create_link_request.from, create_link_request.to);
        ImGui::Text("create_node_request: %c", create_node_request ? 'T' : 'F');
        ImGui::Text("delete_link_requests: %d", delete_link_requests.size());
        ImGui::Text("delete_node_requests: %d", delete_node_requests.size());
        ImGui::Text("screen_offset: %.1f, %.1f", screen_offset.x, screen_offset.y);
        ImGui::Text("node_movement: %.1f, %.1f", node_movement.x, node_movement.y);
        ImGui::Text("mouse_pos: %.1f, %.1f", ImGui::GetMousePos().x, ImGui::GetMousePos().y);
    }
    ImGui::End();

    if (movement_source != invalid_id) {
        for (uint64 id : selection.nodes) {
            if (id == movement_source)
                continue;
            node_states.at(id).world_pos += node_movement;
            node_states.at(id).force_pos = true;
        }
    }

    if (selecting) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        v2 min = v2(math::min(select_start.x, ImGui::GetMousePos().x), math::min(select_start.y, ImGui::GetMousePos().y));
        v2 max = v2(math::max(select_start.x, ImGui::GetMousePos().x), math::max(select_start.y, ImGui::GetMousePos().y));
        draw_list->AddRectFilled(ImVec2(min), ImVec2(max), IM_COL32(255, 255, 255, 20), 0.0f, 0);
        draw_list->AddRect(ImVec2(min), ImVec2(max), IM_COL32(255, 255, 255, 200), 0.0f, 0, 1.0f);
    }

    if (pin_pressed != invalid_id) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        PinState& pin1 = pin_states.at(pin_pressed);
        ImVec2 pin2_pos = ImGui::GetMousePos();
        draw_list->AddLine(ImVec2(pin1.pos), pin2_pos, IM_COL32(255, 255, 255, 255), 2.0f);
    }

    if (Input::mouse_release[GLFW_MOUSE_BUTTON_LEFT]) {
        pin_pressed = invalid_id;
    }
}

bool NodeEditorContext::BeginNode(uint64 id, string_view name) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    active_node = id;
    string window_name = fmt_("{}###{}", zoom_level < 2 ? name : "", id);

    v2 screen_clip_min = v2(ImGui::GetWindowPos()) + v2(ImGui::GetWindowContentRegionMin());
    v2 screen_clip_max = v2(ImGui::GetWindowPos()) + v2(ImGui::GetWindowContentRegionMax());

    if (node_states.contains(id)) {
        v2 screen_pos = WorldToScreen(node_states.at(id).world_pos);
        if (math::contains(range2{screen_clip_min, screen_clip_max}, screen_pos)) {
            node_states.at(id).culled = false;
            if (node_states.at(id).force_pos || math::length(screen_offset_delta) > 0.0f || Input::mouse_wheel != 0.0f)
                ImGui::SetNextWindowPos(ImVec2{screen_pos}, 0, ImVec2{0.5f, 0.5f});
        }
        else {
            node_states.at(id).culled = true;
            return false;
        }
    }

    if (selection.node_selected(id)) {
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(255, 255, 255, 255));
    }
    bool stay_open = true;
    bool ret = ImGui::Begin(window_name.c_str(), &stay_open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking);
    bool moving = false;
    if (ImGui::IsItemActive())
        moving = true;
    if (!stay_open)
        delete_node_requests.push_back(id);

    v2 window_pos = v2(ImGui::GetWindowPos());
    v2 window_size = v2(ImGui::GetWindowSize());

    if (selection.node_selected(id)) {
        draw_list->AddRect(
            ImVec2{window_pos.x - 1, window_pos.y - 1},
            ImVec2{window_pos.x + window_size.x + 1, window_pos.y + window_size.y + 1},
            IM_COL32(255, 255, 255, 255), 3.0f
        );
        ImGui::PopStyleColor(1);
    }

    window_pos += window_size * 0.5f;
    window_pos = ScreenToWorld(window_pos);

    if (moving && node_states.contains(id)) {
        movement_source = id;
        node_movement = window_pos - node_states.at(id).world_pos;
    }

    if (moving || !node_states.contains(id)) {
        node_states[id] = NodeState{.world_pos = v2(window_pos), .size = v2(window_size)};
    }
    return ret;
}

void NodeEditorContext::EndNode() {
    if (!node_states.at(active_node).culled)
        ImGui::End();
    active_node = invalid_id;
}

void NodeEditorContext::BeginPin(uint64 id, PinFlow flow) {
    active_pin = id;
    ImGui::PushID(id);
    ImGui::BeginGroup();

    v2 pin_pos = v2(ImGui::GetCursorScreenPos());
    assert_else(active_node != invalid_id);
    pin_states[id] = {active_node, pin_pos, false, flow};
}

void NodeEditorContext::EndPin(bool ethereal) {
    assert_else(pin_states.contains(active_pin))
        return;
    ImGui::EndGroup();

    bool node_culled = node_states.at(active_node).culled;

    if (!ethereal && !node_culled) {
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0f, ImGui::GetStyle().ItemSpacing.y});
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2{0.0f, ImGui::GetStyle().ItemInnerSpacing.y});

        ImVec2 rect_size = ImGui::GetItemRectSize();
        ImGui::SameLine();
        ImGui::SetCursorPos({ImGui::GetCursorPos().x - rect_size.x, ImGui::GetCursorPos().y});
        ImGui::InvisibleButton("##button", rect_size);

        if (ImGui::IsItemClicked())
            pin_pressed = active_pin;
        else if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
            if (pin_pressed != invalid_id)
                if (pin_states.at(active_pin).flow != pin_states.at(pin_pressed).flow)
                    hovered_pin = active_pin;

        ImGui::PopStyleVar(2);
    }
    ImGui::PopID();


    if (!pin_states.at(active_pin).user_set_pos) {
        if (node_culled) {
            pin_states.at(active_pin).pos = WorldToScreen(node_states.at(active_node).world_pos);
        }
        else if (!ethereal) {
            v2 start_pos = pin_states.at(active_pin).pos;
            v2 end_pos = v2(ImGui::GetCursorScreenPos());
            pin_states.at(active_pin).pos = (start_pos + end_pos) / 2.0f;
        }
        else {
            if (pin_states.at(active_pin).flow == PinFlow_Input)
                pin_states.at(active_pin).pos = v2(ImGui::GetWindowPos()) + v2(ImGui::GetWindowSize()) * v2(0.00f, 0.5f);
            else
                pin_states.at(active_pin).pos = v2(ImGui::GetWindowPos()) + v2(ImGui::GetWindowSize()) * v2(1.00f, 0.5f);
        }
    }

    active_pin = invalid_id;
}

void NodeEditorContext::Link(uint64 id, uint64 from, uint64 to) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    PinState& pin1 = pin_states.at(from);
    PinState& pin2 = pin_states.at(to);

    v2 pin1_pos = pin1.pos;
    v2 pin2_pos = pin2.pos;

    const float tangent_length = (0.2f * math::distance(pin1_pos, pin2_pos) + 30.0f) / _GetScreenSpaceScale();

    draw_list->AddBezierCubic(
        ImVec2(pin1_pos),
        ImVec2(pin1_pos.x - tangent_length, pin1_pos.y),
        ImVec2(pin2_pos.x + tangent_length, pin2_pos.y),
        ImVec2(pin2_pos),
        selection.link_selected(id) ? IM_COL32(255, 255, 255, 255) : IM_COL32(230, 230, 230, 230),
        selection.link_selected(id) ? 3.5f : 1.5f,
        0
    );
    link_states[id] = {.start_id = from, .end_id = to, .start = pin1_pos, .end = pin2_pos};
}

void NodeEditorContext::AcceptCreateLink() {
    create_link_request = {};
}
void NodeEditorContext::RejectCreateLink() {
    create_link_request = {};
}
void NodeEditorContext::AcceptDeleteLink(uint64 id) {

}
void NodeEditorContext::AcceptDeleteNode(uint64 id) {

}
void NodeEditorContext::RejectDeleteLink(uint64 id) {

}
void NodeEditorContext::RejectDeleteNode(uint64 id) {

}

void NodeEditorContext::SetNodeScreenPos(uint64 id, v2 pos) {
    SetNodeWorldPos(id, ScreenToWorld(pos));
}
void NodeEditorContext::SetNodeWorldPos(uint64 id, v2 pos) {
    if (!node_states.contains(id)) {
        node_states[id] = {.force_pos = true, .world_pos = pos};
        return;
    }
    node_states.at(id).force_pos = true;
    node_states.at(id).world_pos = pos;
}

void NodeEditorContext::_DrawBackground() {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    v2 min_screenspace = {ImGui::GetWindowPos().x, ImGui::GetWindowPos().y};
    v2 max_screenspace = {ImGui::GetWindowSize().x + ImGui::GetWindowPos().x, ImGui::GetWindowSize().y + ImGui::GetWindowPos().y};


    const v2 pan_screenspace = WorldToScreen(v2());
    const float size = style.checker_size;

    // This has an issue with unnecessary drawcalls when far away from origin, but I like this code the most
    // Could start by stepping back until we are out of bounds, then doing 1 forward iteration from there
    if (style.draw_checker) {
        v2 current_pan_screenspace = WorldToScreen(v2());

        struct IterOrient { bool a,b; };
        for (auto [reverse_x, reverse_y] : {IterOrient{false, false}, {true, false}, {true, true}, {false, true}})
        for (int32 y = !reverse_y ? 0 : -1; true; y += !reverse_y ? 1 : -1) {
            bool break_y;
            for (int32 x = !reverse_x ? 0 : -1; true; x += !reverse_x ? 1 : -1) {
                v2 grid_start_screenspace = WorldToScreen(v2(x * size, y * size));
                v2 grid_end_screenspace = WorldToScreen(v2((x + 1) * size, (y + 1) * size));
                ImU32 col = ImGui::GetColorU32(ImVec4((math::abs(x%2) ^ math::abs(y%2)) ? style.checker1 : style.checker2));
                draw_list->AddRectFilled(ImVec2(grid_start_screenspace), ImVec2(grid_end_screenspace), col);

                if (!reverse_y)
                    break_y = grid_start_screenspace.y > max_screenspace.y;
                else
                    break_y = grid_start_screenspace.y < min_screenspace.y;
                if (!reverse_x) {
                    if (grid_end_screenspace.x > max_screenspace.x)
                        break;
                }
                else {
                    if (grid_end_screenspace.x < min_screenspace.x)
                        break;
                }
            }
            if (break_y)
                break;
        }
    }

    if (style.draw_dots) {

    }

    if (style.draw_grid) {
        draw_list->AddLine(ImVec2(pan_screenspace.x, 0.0f), ImVec2(pan_screenspace.x, 3000.0f), IM_COL32(255, 255, 255, 10));
        draw_list->AddLine(ImVec2(0.0f, pan_screenspace.y), ImVec2(3000.0f, pan_screenspace.y), IM_COL32(255, 255, 255, 10));
    }
}

void NodeEditorContext::Submit() {
    for (uint64 id : delete_node_requests) {
        for (auto& [link_id, state] : link_states) {
            bool first_matches = pin_states.contains(state.start_id) && pin_states.at(state.start_id).node_id == id;
            bool second_matches = pin_states.contains(state.end_id) && pin_states.at(state.end_id).node_id == id;
            if (first_matches || second_matches)
                if (!delete_link_requests.contains(link_id))
                    delete_link_requests.push_back(link_id);
        }
    }
}

void NodeEditorContext::SetPinCenter(v2 pos) {
    assert_else(active_pin != invalid_id)
        return;
    pin_states.at(active_pin).pos = pos;
    pin_states.at(active_pin).user_set_pos = true;
}

float NodeEditorContext::_GetScreenSpaceScale(int level) {
    int used_level = level;
    if (level == -INT_MAX)
        used_level = zoom_level;
    if (used_level == 0) return 1.0f;
    if (used_level == 1) return 2.0f;
    if (used_level == 2) return 3.0f;
    return 1.0f;
}

v2 NodeEditorContext::ScreenToWorld(const v2& v, int zoom_level) {
    return v * _GetScreenSpaceScale(zoom_level) + screen_offset;
}

v2 NodeEditorContext::WorldToScreen(const v2& v, int zoom_level) {
    return (v - screen_offset) / _GetScreenSpaceScale(zoom_level);
}

}