#pragma once

#include "general/string.hpp"

namespace spellbook {

struct Scene;
struct Timer;

struct TimerCallback {
    void(*callback)(Timer*, void*) = {};
    Timer* timer;
    void* payload = nullptr;
};

struct Timer {
    string name;
    float total_time;
    TimerCallback callback;
    bool trigger_every_tick = false;

    Scene* scene;
    float remaining_time;
    float time_scale = 1.0f;
    bool ticking = false;


    void start(float time = -1.0f);
    void update(float delta);
    void timeout();
    void stop();
};

Timer& add_timer(Scene* scene, string name, void(*callback)(Timer*, void*) = {}, void* payload = nullptr, float time = -1.0f);
Timer& add_tween_timer(Scene* scene, string name, void(*callback)(Timer*, void*) = {}, void* payload = nullptr, float time = -1.0f);
void remove_timer(Scene* scene, string name);
void inspect(Timer* timer);

}