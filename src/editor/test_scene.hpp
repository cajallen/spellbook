#pragma once

#include "general/vector.hpp"
#include "game/scene.hpp"

namespace spellbook {

struct TestScene {
    vector<v3> control_points;
    s32 degrees;
    float t;
    
    Scene* p_scene        = nullptr;
    
    void setup();
    void update();
    void window(bool* p_open);
    void shutdown();
};

}