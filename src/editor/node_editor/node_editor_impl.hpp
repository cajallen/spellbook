#pragma once

#include "general/string.hpp"
#include "general/color.hpp"
#include "general/id_ptr.hpp"
#include "general/memory.hpp"

#include "editor/editor_scene.hpp"

#include "editor/node_editor/node_editor.hpp"

namespace spellbook {

struct Node;
struct Pin;

struct Link {
    id_ptr<Link> id;
    id_ptr<unique_ptr<Pin>> input;
    id_ptr<unique_ptr<Pin>> output;
};

struct Pin {
    NodeEditorContext& context;

    id_ptr<unique_ptr<Pin>> id;
    string name;
    uint64 type;
    PinFlow flow;
    bool exclusive;
    vector<id_ptr<Link>> links;

protected:
    bool circle_enabled = true;
    bool text_enabled = true;

public:
    Pin(NodeEditorContext& ctx) : context(ctx) {}

    virtual void show_setup();
    virtual float calc_width();
    virtual void show();
    virtual json_value serialize();

    string get_widget_title();
    void show_circle(bool widget_height = false);

    virtual constexpr Color get_color() { return Color::hsv(0.0f, 0.8f, 0.8f, 1.0f); }
};

struct Node {
    NodeEditorContext& context;

    id_ptr<unique_ptr<Node>> id;
    string name;
    Color color = palette::white;

    vector<id_ptr<unique_ptr<Pin>>> in_pins;
    vector<id_ptr<unique_ptr<Pin>>> out_pins;

    Node(NodeEditorContext& ctx) : context(ctx) {}
    virtual string get_window_name();
    virtual float calc_width();
    void show();
    virtual json_value serialize();

    template<typename T>
    id_ptr<unique_ptr<Pin>> create_pin(string_view pin_name, PinFlow flow, bool excl) {
        id_ptr<unique_ptr<Pin>> pin_id;
        if (flow == PinFlow_Input)
            pin_id = in_pins.push_back(id_ptr<unique_ptr<Pin>>::emplace());
        else if (flow == PinFlow_Output)
            pin_id = out_pins.push_back(id_ptr<unique_ptr<Pin>>::emplace());
        *pin_id = make_unique<T>(context);
        (*pin_id)->type = typeid(T).hash_code();
        (*pin_id)->flow = flow;
        (*pin_id)->id = pin_id.id;
        (*pin_id)->name = pin_name;
        (*pin_id)->exclusive = excl;

        return pin_id;
    }

    virtual constexpr Color get_color() { return Color::hsv(0.0f, 0.8f, 0.8f, 1.0f); }
    static constexpr uint32 get_id() { return hash_view("Node"); }
};


struct NodeEditor : EditorScene {
    NodeEditorContext* node_context;
    vector<id_ptr<unique_ptr<Node>>> nodes;
    vector<id_ptr<Link>> links;

    void setup() override;
    void update() override;
    void window(bool* p_open) override;
    void shutdown() override;

    virtual id_ptr<unique_ptr<Node>> menu_create_node_list() = 0;

    void handle_popups();
    void handle_create();
    void handle_delete();

    void delete_link(id_ptr<Link>& link);
    void delete_node(id_ptr<unique_ptr<Node>>& node);

    void push_style();
    void pop_style();

    template<typename T>
    id_ptr<unique_ptr<Node>> create_node() {
        auto node_id = id_ptr<unique_ptr<Node>>::emplace();
        unique_ptr<Node>& node = *node_id;
        node = make_unique<T>(*node_context);
        node->id = node_id.id;
        node->name = string(typeid(T).name());
        node->name = node->name.substr(node->name.find("::Node") + sizeof("::Node") - 1);
        node->color = node->get_color();

        nodes.push_back(node_id);

        return node_id;
    }
    virtual id_ptr<unique_ptr<Node>> deserialize_node(const json& j);
};

#define NODE_DESERIALIZE_IF_TYPE(T) if (T::get_id() == type_id) node = create_node<T>();

}