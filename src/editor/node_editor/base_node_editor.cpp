#include "base_node_editor.hpp"

#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "extension/fmt.hpp"

namespace spellbook {

NodeFloat::NodeFloat(NodeEditorContext& ctx) : Node(ctx) {
    create_pin<PinFloat>("", PinFlow_Output, false);
}
NodeFloatRange::NodeFloatRange(NodeEditorContext& ctx) : Node(ctx) {
    create_pin<PinFloatRange>("", PinFlow_Output, false);
}
NodeInt::NodeInt(NodeEditorContext& ctx) : Node(ctx) {
    create_pin<PinInt>("", PinFlow_Output, false);
}
NodeIntRange::NodeIntRange(NodeEditorContext& ctx) : Node(ctx) {
    create_pin<PinIntRange>("", PinFlow_Output, false);
}
NodeString::NodeString(NodeEditorContext& ctx) : Node(ctx) {
    create_pin<PinString>("", PinFlow_Output, false);
}

void PinWidget::show_setup() {
    circle_enabled = true;

    bool has_input = flow == PinFlow_Input && !links.empty();
    widget_enabled = context.zoom_level == 0 && !has_input;
    text_enabled = !widget_enabled;
}

float PinWidget::calc_width() {
    float width = 0.0f;
    if (widget_enabled)
        width += get_widget_width() + context.style.item_spacing[context.zoom_level];
    width += Pin::calc_width();
    return width;
}

void PinEvent::show() {
    if (flow == PinFlow_Input) {
        Pin::show();
    }

    if (flow == PinFlow_Output) {
        Pin::show();
    }
}

void PinFloat::show() {
    if (flow == PinFlow_Input) {
        Pin::show();
        if (widget_enabled) {
            string format_string = !name.empty() ? fmt_("{}   %.2f", name) : "%.2f";
            ImGui::SameLine();
            ImGui::PushItemWidth(get_widget_width());
            ImGui::DragFloat(get_widget_title().c_str(), &backup, 0.01f, -FLT_MAX, FLT_MAX, "%.2f");
        }
    }

    if (flow == PinFlow_Output) {
        if (widget_enabled) {
            string format_string = !name.empty() ? fmt_("%.2f   {}", name) : "%.2f";
            ImGui::PushItemWidth(get_widget_width());
            ImGui::DragFloat(get_widget_title().c_str(), &backup, 0.01f, -FLT_MAX, FLT_MAX, "%.2f");
            ImGui::SameLine();
        }
        Pin::show();
    }
}

void PinFloatRange::show() {
    if (flow == PinFlow_Input) {
        Pin::show();
        if (widget_enabled) {
            ImGui::SameLine();
            ImGui::PushItemWidth(get_widget_width());
            ImGui::DragFloat2(get_widget_title().c_str(), backup.data, 0.01f, -FLT_MAX, FLT_MAX, "%.2f");
            ImGui::PopItemWidth();
        }
    }

    if (flow == PinFlow_Output) {
        if (widget_enabled) {
            ImGui::PushItemWidth(get_widget_width());
            ImGui::DragFloat2(get_widget_title().c_str(), backup.data, 0.01f, -FLT_MAX, FLT_MAX, "%.2f");
            ImGui::PopItemWidth();
            ImGui::SameLine();
        }
        Pin::show();
    }
}

void PinInt::show() {
    if (flow == PinFlow_Input) {
        Pin::show();
        if (widget_enabled) {
            ImGui::SameLine();
            ImGui::PushItemWidth(get_widget_width());
            ImGui::DragInt(get_widget_title().c_str(), &backup, 0.01f, -INT_MAX, INT_MAX, "%d");
        }
    }

    if (flow == PinFlow_Output) {
        if (widget_enabled) {
            ImGui::PushItemWidth(get_widget_width());
            ImGui::DragInt(get_widget_title().c_str(), &backup, 0.01f, -INT_MAX, INT_MAX, "%d");
            ImGui::SameLine();
        }
        Pin::show();
    }
}

void PinIntRange::show() {
    if (flow == PinFlow_Input) {
        Pin::show();
        if (widget_enabled) {
            ImGui::SameLine();
            ImGui::PushItemWidth(get_widget_width());
            ImGui::DragInt2(get_widget_title().c_str(), backup.data, 0.01f, -INT_MAX, INT_MAX, "%d");
            ImGui::PopItemWidth();
        }
    }

    if (flow == PinFlow_Output) {
        if (widget_enabled) {
            ImGui::PushItemWidth(get_widget_width());
            ImGui::DragInt2(get_widget_title().c_str(), backup.data, 0.01f, -INT_MAX, INT_MAX, "%d");
            ImGui::PopItemWidth();
            ImGui::SameLine();
        }
        Pin::show();
    }
}

void PinString::show() {
    if (flow == PinFlow_Input) {
        Pin::show();
        if (widget_enabled) {
            ImGui::PushItemWidth(get_widget_width());
            ImGui::InputText(get_widget_title().c_str(), &backup);
            ImGui::PopItemWidth();
            ImGui::SameLine();
        }
    }

    if (flow == PinFlow_Output) {
        if (widget_enabled) {
            ImGui::PushItemWidth(get_widget_width());
            ImGui::InputText(get_widget_title().c_str(), &backup);
            ImGui::PopItemWidth();
            ImGui::SameLine();
        }
        Pin::show();
    }
}

id_ptr<unique_ptr<Node>> BaseNodeEditor::deserialize_node(const json& j) {
    id_ptr<unique_ptr<Node>> node;
    uint32 type_id = from_jv<uint32>(*j.at("type"));
    NODE_DESERIALIZE_IF_TYPE(NodeFloat);
    NODE_DESERIALIZE_IF_TYPE(NodeFloatRange);
    NODE_DESERIALIZE_IF_TYPE(NodeInt);
    NODE_DESERIALIZE_IF_TYPE(NodeIntRange);
    NODE_DESERIALIZE_IF_TYPE(NodeString);

    if (node.valid())
        return node;

    return NodeEditor::deserialize_node(j);
}


id_ptr<unique_ptr<Node>> BaseNodeEditor::menu_create_node_list() {
    id_ptr<unique_ptr<Node>> node;

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(PinFloat(*node_context).get_color()));
    if (ImGui::MenuItem("Float"))
        node = create_node<NodeFloat>();

    if (ImGui::MenuItem("FloatRange"))
        node = create_node<NodeFloatRange>();

    if (ImGui::MenuItem("Int"))
        node = create_node<NodeInt>();

    if (ImGui::MenuItem("IntRange"))
        node = create_node<NodeIntRange>();
    ImGui::PopStyleColor();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(PinString(*node_context).get_color()));
    if (ImGui::MenuItem("String"))
        node = create_node<NodeString>();
    ImGui::PopStyleColor();

    return node;
}

}

