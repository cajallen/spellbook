#pragma once

#include "color.hpp"
#include "vector.hpp"
#include "geometry.hpp"
#include "string.hpp"
#include "scene.hpp"

#include "renderer/prefab.hpp"

namespace spellbook {

struct Tower {
    string name;
    Color color;
    vector<u32> r_indices;
    PrefabCPU* prefab;
};

struct Brush {
    string             name;
    Color              color;
    vector<Renderable> model;
    bool               travelable;
};

struct MapEditor {
    static vector<Brush> brushes;
    static vector<Tower> towers;

    int    selected_brush = -1;
    int    selected_tower = -1;
    Scene* p_scene = nullptr;
    void   setup();
    void   setup(Scene* init_scene);
    void   update();
    void   window(bool* p_open);
};

}
