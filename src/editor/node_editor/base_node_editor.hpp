#pragma once

#include "node_editor_impl.hpp"

namespace spellbook {

struct NodeFloat : Node {
    NodeFloat(NodeEditorContext& ctx);
    constexpr Color get_color() override { return Color::hsv(150.0f, 0.8f, 0.6f, 1.0f); }
    static constexpr uint32 get_id() { return hash_view("Float"); }
};
struct NodeFloatRange : Node {
    NodeFloatRange(NodeEditorContext& ctx);
    constexpr Color get_color() override { return Color::hsv(150.0f, 0.8f, 0.6f, 1.0f); }
    static constexpr uint32 get_id() { return hash_view("Float Range"); }
};
struct NodeInt : Node {
    NodeInt(NodeEditorContext& ctx);
    constexpr Color get_color() override { return Color::hsv(150.0f, 0.8f, 0.6f, 1.0f); }
    static constexpr uint32 get_id() { return hash_view("Int"); }
};
struct NodeIntRange : Node {
    NodeIntRange(NodeEditorContext& ctx);
    constexpr Color get_color() override { return Color::hsv(150.0f, 0.8f, 0.6f, 1.0f); }
    static constexpr uint32 get_id() { return hash_view("Int Range"); }
};
struct NodeString : Node {
    NodeString(NodeEditorContext& ctx);
    constexpr Color get_color() override { return Color::hsv(150.0f, 0.6f, 0.8f, 1.0f); }
    static constexpr uint32 get_id() { return hash_view("String"); }
};

struct PinWidget : Pin {
    bool widget_enabled = false;
    PinWidget(NodeEditorContext& ctx) : Pin(ctx) {}
    constexpr virtual float get_widget_width() { return 70.0f; }
    void show_setup() override;
    float calc_width() override;
};

struct PinFloat : PinWidget {
    float backup = 0.0f;
    PinFloat(NodeEditorContext& ctx) : PinWidget(ctx) {}
    void show() override;
    constexpr float get_widget_width() override { return 70.0f; }
    constexpr Color get_color() override { return Color::hsv(150.0f, 0.8f, 0.6f, 1.0f); }
};
struct PinFloatRange : PinWidget {
    v2 backup = {};
    PinFloatRange(NodeEditorContext& ctx) : PinWidget(ctx) {}
    void show() override;
    constexpr float get_widget_width() override { return 90.0f; }
    constexpr Color get_color() override { return Color::hsv(150.0f, 0.8f, 0.6f, 1.0f); }
};
struct PinInt : PinWidget {
    int backup = 0;
    PinInt(NodeEditorContext& ctx) : PinWidget(ctx) {}
    void show() override;
    constexpr float get_widget_width() override { return 70.0f; }
    constexpr Color get_color() override { return Color::hsv(150.0f, 0.8f, 0.6f, 1.0f); }
};
struct PinIntRange : PinWidget {
    v2i backup = {};
    PinIntRange(NodeEditorContext& ctx) : PinWidget(ctx) {}
    void show() override;
    constexpr float get_widget_width() override { return 90.0f; }
    constexpr Color get_color() override { return Color::hsv(150.0f, 0.8f, 0.6f, 1.0f); }
};
struct PinString : PinWidget {
    string backup;
    PinString(NodeEditorContext& ctx) : PinWidget(ctx) {}
    void show() override;
    constexpr float get_widget_width() override { return 110.0f; }
    constexpr Color get_color() override { return Color::hsv(150.0f, 0.8f, 0.6f, 1.0f); }
};
struct PinEvent : PinWidget {
    PinEvent(NodeEditorContext& ctx) : PinWidget(ctx) {}
    void show() override;
    constexpr float get_widget_width() override { return 110.0f; }
    constexpr Color get_color() override { return Color::hsv(150.0f, 0.0f, 0.0f, 1.0f); }
};


struct BaseNodeEditor : NodeEditor {
    id_ptr<unique_ptr<Node>> menu_create_node_list() override;
    virtual id_ptr<unique_ptr<Node>> deserialize_node(const json& j);
};

}