#pragma once

#include <functional>
#include "general/string.hpp"

namespace spellbook {

struct Scene;
struct Timer;

struct TimerCallback {
    Timer* timer;
    std::function<void(Timer*)> callback = {};
};

struct Timer {
    string name;
    Scene* scene;
    TimerCallback callback;
    bool trigger_every_tick = false;
    float total_time;

    float remaining_time;
    float time_scale = 1.0f;
    bool ticking = false;
    bool one_shot = true;

    void start(float time = -1.0f);
    void update(float delta);
    void timeout();
    void stop();
};

void update_timers(Scene* scene);

Timer& add_timer(Scene* scene, const string& name, std::function<void(Timer*)> callback = {}, bool permanent = false);
Timer& add_tween_timer(Scene* scene, const string& name, std::function<void(Timer*)> callback = {}, bool permanent = false);
void destroy_timer(Timer* timer);
void remove_timer(Scene* scene, Timer* timer);
void remove_timer(Scene* scene, const string& name);
void inspect(Timer* timer);

}