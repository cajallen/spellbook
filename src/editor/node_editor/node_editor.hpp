#pragma once

#include "general/umap.hpp"
#include "general/color.hpp"
#include "general/math/geometry.hpp"

namespace spellbook {

enum PinFlow {
    PinFlow_Input,
    PinFlow_Output
};

struct NodeEditorContext {
    uint64 pin_pressed;
    uint64 hovered_pin;

    uint64 active_node;
    uint64 active_pin;

    struct LinkRequest {
        uint64 from;
        uint64 to;
    };
    LinkRequest create_link_request;
    bool create_node_request;

    vector<uint64> delete_link_requests;
    vector<uint64> delete_node_requests;

    v2 screen_offset_delta;
    v2 screen_offset;
    int zoom_level;

    bool selecting = false;
    v2 select_start;

    uint64 movement_source;
    v2 node_movement;

    struct NodeState {
        bool force_pos;
        v2 world_pos;
        v2 size;
        bool culled = false;
    };
    umap<uint64, NodeState> node_states;

    struct PinState {
        uint64 node_id;
        v2 pos;
        bool user_set_pos;
        PinFlow flow;
    };
    umap<uint64, PinState> pin_states;

    struct LinkState {
        uint64 start_id;
        uint64 end_id;
        v2 start;
        v2 end;
    };
    umap<uint64, LinkState> link_states;

    struct Selection {
        vector<uint64> nodes;
        vector<uint64> links;

        void clear() { nodes.clear(); links.clear(); }
        bool node_selected(uint64 id) { return nodes.contains(id); }
        bool link_selected(uint64 id) { return links.contains(id); }
    } selection;

    struct Style {
        Color checker1 = Color(40.0f / 255.0f, 1.0f);
        Color checker2 = Color(45.0f / 255.0f, 1.0f);
        float checker_size = 24.0f;
        bool draw_checker = true;
        bool draw_dots = false;
        bool draw_grid = true;

        float item_spacing[3] = {7.0f, 4.0f, 1.0f};
    } style;

    void Begin();
    void End();

    bool BeginNode(uint64 id, string_view name);
    void EndNode();
    void SetNodeScreenPos(uint64 id, v2 pos);
    void SetNodeWorldPos(uint64 id, v2 pos);

    void BeginPin(uint64 id, PinFlow flow);
    void SetPinCenter(v2 pos);
    void EndPin(bool ethereal = false);

    void Link(uint64 id, uint64 from, uint64 to);

    void Submit();

    void AcceptCreateLink();
    void RejectCreateLink();
    void AcceptDeleteLink(uint64 id);
    void AcceptDeleteNode(uint64 id);
    void RejectDeleteLink(uint64 id);
    void RejectDeleteNode(uint64 id);

    void _DrawBackground();
    float _GetScreenSpaceScale(int zoom_level = -INT_MAX);

    v2 ScreenToWorld(const v2& v, int zoom_level = -INT_MAX);
    v2 WorldToScreen(const v2& v, int zoom_level = -INT_MAX);

    static constexpr uint64 invalid_id = 0;
};

}