#pragma once

#include "editor/editor_scene.hpp"
#include "game/scene.hpp"
#include "game/map.hpp"
#include "game/gui/gui_manager.hpp"

namespace spellbook {

struct Font;

struct HitTestScene : EditorScene {
    MapPrefab map_prefab;

    GUIManager gui_manager;

    void setup() override;
    void setup_scene(Scene* scene, bool scene_setup);
    void update() override;
    void window(bool* p_open) override;

    void show_buttons();

    void build_visuals(Scene* scene);

    string text = "Grants {tip=haste1}Haste 1{\\tip} to the caster";
};

}