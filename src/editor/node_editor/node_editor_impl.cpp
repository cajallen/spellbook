#include "node_editor_impl.hpp"

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include "extension/icons/font_awesome4.h"
#include "extension/fmt.hpp"

namespace spellbook {


string Node::get_window_name() {
    if (context.zoom_level == 2)
        return "";
    string window_name;
    window_name = fmt_("{}", name);
    return window_name;
}

float Node::calc_width() {
    float in_width_max = 0.0f;
    float out_width_max = 0.0f;

    for (id_ptr<unique_ptr<Pin>> pin_id : in_pins) {
        unique_ptr<Pin>& pin = *pin_id;
        in_width_max = math::max(pin->calc_width(), in_width_max);
    }
    for (id_ptr<unique_ptr<Pin>> pin_id : out_pins) {
        unique_ptr<Pin>& pin = *pin_id;
        out_width_max = math::max(pin->calc_width(), out_width_max);
    }

    float width = in_width_max + out_width_max + ImGui::GetStyle().WindowPadding.x * 2.0f;
    if (context.zoom_level <= 0) {
        width += context.style.item_spacing[context.zoom_level] * 2 + 10.0f;
    }
    else {
        width += context.style.item_spacing[context.zoom_level];
    }

    float title_width = ImGui::CalcTextSize(get_window_name().c_str()).x + 30.0f;
    return math::max(width, title_width);
}

void Node::show() {
    for (uint32 i = 0; i < in_pins.size(); i++) {
        id_ptr<unique_ptr<Pin>> pin_id = in_pins[i];
        (*pin_id)->show_setup();
    }

    for (uint32 i = 0; i < out_pins.size(); i++) {
        id_ptr<unique_ptr<Pin>> pin_id = out_pins[i];
        (*pin_id)->show_setup();
    }

    const float window_width = calc_width();
    ImGui::SetNextWindowSizeConstraints(ImVec2{window_width, 0.0f}, ImVec2{window_width, 1000.0f});

    const int style_var_count = 0;
    const int style_col_count = 3;

    Color background_color = mix(color, palette::black, 0.2f);
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(background_color));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(color));
    ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(Color(background_color, 0.6f)));

    if (context.BeginNode(id.id, get_window_name())) {
        ImGui::PushID(id.id);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {context.style.item_spacing[context.zoom_level], 4.0f});
        ImGui::BeginGroup();
        for (uint32 i = 0; i < in_pins.size(); i++) {
            id_ptr<unique_ptr<Pin>> pin_id = in_pins[i];
            unique_ptr<Pin>& pin = *pin_id;

            context.BeginPin(pin_id.id, PinFlow_Input);
            pin->show();
            context.EndPin();
        }
        if (in_pins.empty()) {
            ImGui::Dummy(ImVec2{0.0f, 0.0f});
        }
        ImGui::EndGroup();


        if (context.zoom_level == 0) {
            ImGui::SameLine();
            ImGui::Dummy(ImVec2 {10.0f, 0.0f});
        }

        ImGui::SameLine();

        ImGui::BeginGroup();
        for (uint32 i = 0; i < out_pins.size(); i++) {
            id_ptr<unique_ptr<Pin>> pin_id = out_pins[i];
            unique_ptr<Pin>& pin = *pin_id;

            context.BeginPin(pin_id.id, PinFlow_Output);
            pin->show();
            context.EndPin();
        }
        if (out_pins.empty()) {
            ImGui::Dummy(ImVec2{0.0f, 0.0f});
        }
        ImGui::EndGroup();
        ImGui::PopStyleVar();
        ImGui::PopID();
        context.EndNode();
    } else {
        for (uint32 i = 0; i < in_pins.size(); i++) {
            id_ptr<unique_ptr<Pin>> pin_id = in_pins[i];
            context.BeginPin(pin_id.id, PinFlow_Input);
            context.EndPin(true);
        }

        for (uint32 i = 0; i < out_pins.size(); i++) {
            id_ptr<unique_ptr<Pin>> pin_id = out_pins[i];
            context.BeginPin(pin_id.id, PinFlow_Output);
            context.EndPin(true);
        }
        context.EndNode();
    }

    ImGui::PopStyleVar(style_var_count);
    ImGui::PopStyleColor(style_col_count);

}

json_value Pin::serialize() {
    return {};
}

json_value Node::serialize() {
    auto j = json();

    vector<json_value> _list = {};
    for (id_ptr<unique_ptr<Pin>> pin_id : in_pins) {
        _list.push_back((*pin_id)->serialize());
    }
    json_value jv;
    jv.value = json_variant{_list};
    j["in_pins"] = make_shared<json_value>(jv);

    return to_jv(j);
}

void NodeEditor::handle_create() {
    if (node_context->create_link_request.from && node_context->create_link_request.to) {
        id_ptr<unique_ptr<Pin>> input = {node_context->create_link_request.from};
        id_ptr<unique_ptr<Pin>> output = {node_context->create_link_request.to};

        bool both_valid = input.valid() && output.valid();
        bool sockets_compatible = false;
        if (both_valid)
            sockets_compatible = (*input)->type == (*output)->type;
        if (both_valid && sockets_compatible) {
            node_context->AcceptCreateLink();

            if ((*input)->exclusive) {
                for (id_ptr<Link> old_link : (*input)->links) {
                    if (old_link.valid()) {
                        node_context->delete_link_requests.push_back(old_link.id);
                    }
                }
            }
            if ((*output)->exclusive) {
                for (id_ptr<Link> old_link : (*output)->links) {
                    if (old_link.valid()) {
                        node_context->delete_link_requests.push_back(old_link.id);
                    }
                }
            }

            auto link    = id_ptr<Link>::emplace();
            (*input)->links.push_back(link);
            (*output)->links.push_back(link);
            link->id     = link.id;
            link->input  = input;
            link->output = output;
            links.push_back(link);


            node_context->Link(link.id, link->input.id, link->output.id);
        } else {
            node_context->RejectCreateLink();
        }
    }
}

void NodeEditor::delete_link(id_ptr<Link>& link) {
    if (link->input.valid())
        (*link->input)->links.remove_value(link);
    if (link->output.valid())
        (*link->output)->links.remove_value(link);
    uint32 index = links.index(link);
    if (index != -1) {
        links.remove_index(links.index(link));
    } else {
        for (id_ptr<Link> &it_link: links) {
            if (it_link.id == link.id) {
                links.remove_index(links.index(it_link));
                break;
            }
        }
    }
}
void NodeEditor::delete_node(id_ptr<unique_ptr<Node>>& node) {
    nodes.remove_index(nodes.index(node));
}


void NodeEditor::handle_delete() {
    for (uint64 delete_link : node_context->delete_link_requests) {
        node_context->AcceptDeleteLink(delete_link);
        for (auto& link : links) {
            if (link.id == delete_link) {
                this->delete_link(link);
                break;
            }
        }
    }

    for (uint64 delete_node : node_context->delete_node_requests) {
        node_context->AcceptDeleteNode(delete_node);
        for (auto& node : nodes) {
            if (node.id == delete_node) {
                this->delete_node(node);
                break;
            }
        }
    }
}

void NodeEditor::handle_popups() {
    auto openPopupPosition = ImGui::GetMousePos();
    if (node_context->create_node_request) {
        ImGui::OpenPopup("New Node");
    }
    if (ImGui::BeginPopup("New Node")) {
        auto new_position = openPopupPosition;

        id_ptr<unique_ptr<Node>> node_id = menu_create_node_list();

        if (node_id != 0)
            node_context->SetNodeScreenPos(node_id.id, v2{new_position.x, new_position.y});
        ImGui::EndPopup();
    }
}

void NodeEditor::setup() {
    node_context = new NodeEditorContext();
}
void NodeEditor::window(bool* p_open) {
    if (ImGui::Begin("Node Editor")) {
        push_style();
        node_context->Begin();
        for (id_ptr<unique_ptr<Node>>& node_id : nodes) {
            unique_ptr<Node>& node = *node_id;
            node->show();
        }

        for (id_ptr<Link>& link : links) {
            node_context->Link(link.id, link->input.id, link->output.id);
        }

        node_context->Submit();

        handle_popups();
        handle_create();
        handle_delete();
        node_context->End();
        pop_style();
    }
    ImGui::End();
}

void NodeEditor::shutdown() {
    delete node_context;
    //p_scene->cleanup();
}

void NodeEditor::update() {
    //p_scene->update();
}

void Pin::show_setup() {
    circle_enabled = true;
    text_enabled = context.zoom_level <= 1;
}

float Pin::calc_width() {
    const int item_count = circle_enabled ? 1 : 0 + text_enabled ? 1 : 0;
    float width = 0.0f;
    if (circle_enabled)
        width += ImGui::CalcTextSize(ICON_FA_CIRCLE_O).x;
    if (text_enabled)
        width += ImGui::CalcTextSize(name.c_str()).x;
    return width + math::max(item_count - 1, 0) * context.style.item_spacing[context.zoom_level];
}
void Pin::show() {
    if (flow == PinFlow_Input) {
        if (circle_enabled)
            show_circle(context.zoom_level == 0);
        if (circle_enabled && text_enabled)
            ImGui::SameLine();
        if (text_enabled) {
            if (context.zoom_level == 0)
                ImGui::AlignTextToFramePadding();
            ImGui::Text("%s", name.c_str());
        }
    }

    if (flow == PinFlow_Output) {
        if (text_enabled) {
            if (context.zoom_level == 0)
                ImGui::AlignTextToFramePadding();
            ImGui::Text("%s", name.c_str());
        }
        if (circle_enabled && text_enabled)
            ImGui::SameLine();
        if (circle_enabled)
            show_circle(context.zoom_level == 0);
    }

    // Allow inheritors to not care if we're doing something or not
    if (!circle_enabled && !text_enabled)
        ImGui::Dummy({0.0f, 0.0f});
}


void Pin::show_circle(bool widget_height) {
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(this->get_color()));
    if (flow == PinFlow_Output && context.zoom_level >= 1) {
        const float pin_width = ImGui::CalcTextSize(ICON_FA_CIRCLE_O).x + ImGui::GetStyle().ItemSpacing.x;
        const float avail_width = ImGui::GetContentRegionAvail().x;
        ImGui::Dummy({avail_width-pin_width, 0.0f});
        ImGui::SameLine();
    }
    if (widget_height)
        ImGui::AlignTextToFramePadding();
    ImGui::Text(ICON_FA_CIRCLE_O);
    v2 text_center = 0.5f * (v2(ImGui::GetItemRectMin()) + v2(ImGui::GetItemRectMax()));
    context.SetPinCenter(text_center);
    ImGui::PopStyleColor(1);
}
string Pin::get_widget_title() {
    return fmt_("##{}", name);
}

id_ptr<unique_ptr<Node>> NodeEditor::deserialize_node(const json& j) {
    id_ptr<unique_ptr<Node>> node;
    return node;
}


void NodeEditor::push_style() {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 3.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2{0.5f, 0.5f});

    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(Color(palette::node_editor_gray, 0.5f))); // ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(Color(palette::node_editor_7, 0.6f))); // ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(Color(palette::node_editor_4, 0.4f))); // ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(Color(palette::node_editor_4, 0.7f))); // ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(Color(palette::node_editor_4, 1.0f))); // ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(Color(palette::node_editor_5, 1.0f))); // ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(Color(palette::node_editor_4, 1.0f))); // ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(Color(palette::node_editor_4, 0.4f))); // ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(Color(palette::node_editor_4, 1.0f))); // ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(Color(palette::node_editor_6, 1.0f))); // ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(Color(palette::node_editor_4, 0.3f))); // ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(Color(palette::node_editor_4, 0.8f))); // ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(Color(palette::node_editor_4, 1.0f))); // ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    ImGui::PushStyleColor(ImGuiCol_SeparatorHovered, ImVec4(Color(palette::node_editor_7, 0.8f))); // ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
    ImGui::PushStyleColor(ImGuiCol_SeparatorActive, ImVec4(Color(palette::node_editor_7, 1.0f))); // ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
    ImGui::PushStyleColor(ImGuiCol_ResizeGrip, ImVec4(Color(palette::node_editor_4, 0.2f))); // ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
    ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, ImVec4(Color(palette::node_editor_4, 0.7f))); // ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
    ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, ImVec4(Color(palette::node_editor_4, 0.9f))); // ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
    ImGui::PushStyleColor(ImGuiCol_DockingPreview, ImVec4(Color(palette::node_editor_4, 0.7f)));
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, ImVec4(Color(palette::node_editor_4, 0.3f))); // ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
    ImGui::PushStyleColor(ImGuiCol_NavHighlight, ImVec4(Color(palette::node_editor_4, 1.0f))); // ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    ImGui::PushStyleColor(ImGuiCol_Separator, ImGui::GetStyleColorVec4(ImGuiCol_Border));
    ImGui::PushStyleColor(ImGuiCol_TabHovered, ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
    ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(mix(
        Color(ImGui::GetStyleColorVec4(ImGuiCol_Header)),
        Color(ImGui::GetStyleColorVec4(ImGuiCol_TitleBgActive)),
        0.80f
    )));
    ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(mix(
        Color(ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive)),
        Color(ImGui::GetStyleColorVec4(ImGuiCol_TitleBgActive)),
        0.60f
    )));
    ImGui::PushStyleColor(ImGuiCol_TabUnfocused, ImVec4(mix(
        Color(ImGui::GetStyleColorVec4(ImGuiCol_Tab)),
        Color(ImGui::GetStyleColorVec4(ImGuiCol_TitleBg)),
        0.80f
    )));
    ImGui::PushStyleColor(ImGuiCol_TabUnfocusedActive, ImVec4(mix(
        Color(ImGui::GetStyleColorVec4(ImGuiCol_TabActive)),
        Color(ImGui::GetStyleColorVec4(ImGuiCol_TitleBg)),
        0.40f
    )));
}

void NodeEditor::pop_style() {
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(27);
}


}