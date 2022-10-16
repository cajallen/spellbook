#pragma once

#include "color.hpp"
#include "vector.hpp"
#include "geometry.hpp"
#include "string.hpp"
#include "scene.hpp"

#include "game/tower.hpp"
#include "renderer/assets/model.hpp"

namespace spellbook {

template <typename T>
struct Button {
    string text;
    Color color;
    T* item;
};

struct MapEditor {
    vector<Button<Tower>> towers;
    int    selected_tower = -1;
    u32 selected_brush;
    Scene* p_scene        = nullptr;
    void   setup();
    void   setup(Scene* init_scene);
    void   update();
    void   window(bool* p_open);
};

}
