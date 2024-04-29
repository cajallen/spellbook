#pragma once

namespace spellbook {

struct GUIManager;
struct Player;
struct RenderScene;

struct HUDGUI {
    bool enabled = false;
    Player* player = nullptr;

    void draw(GUIManager* manager, RenderScene& render_scene);
    void update(GUIManager* manager);
};

}