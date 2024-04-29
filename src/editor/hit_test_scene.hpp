#pragma once

#include "editor/editor_scene.hpp"
#include "game/scene.hpp"
#include "game/map.hpp"
#include "game/gui/gui_manager.hpp"

namespace spellbook {

struct Font;

struct HitTestScene : EditorScene {
    MapPrefab map_prefab;

    struct HitEffectInfo {
        bool enabled;
        int32 level;
        float quantity;
        float duration;
    };
    umap<string, HitEffectInfo> effects;

    entt::entity god_caster;

    void setup() override;
    void setup_scene(Scene* scene, bool scene_setup);
    void update() override;
    void window(bool* p_open) override;

    void build_visuals(Scene* scene);
};

}