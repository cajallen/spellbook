#pragma once

#include "general/string.hpp"

namespace spellbook {

struct Scene;

struct TimerCallback {
    void(*callback)(void*) = {};
    void* payload = nullptr;
};

struct Timer {
    string name;
    float total_time;
    TimerCallback callback;

    Scene* scene;
    float remaining_time;
    float time_scale = 1.0f;
    bool ticking = false;

    void start(float time = -1.0f);
    void update(float delta);
    void timeout();
    void stop();
};

Timer& add_timer(Scene* scene, string name, void(*callback)(void*) = {}, void* payload = nullptr, float time = -1.0f);
void remove_timer(Scene* scene, string name);
void inspect(Timer* timer);

}