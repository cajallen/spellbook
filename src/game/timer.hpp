#pragma once

#include <functional>
#include <memory>
#include "general/string.hpp"
#include "general/vector.hpp"

namespace spellbook {

struct Scene;
struct Timer;

struct TimerCallback {
    Timer* timer;
    std::function<void(Timer*)> callback = {};
};

struct Timer {
    Scene* scene;
    TimerCallback callback;
    bool trigger_every_tick = false;
    float total_time;

    float remaining_time;
    float time_scale = 1.0f;
    bool ticking = false;
    
    // Don't delete while running even if all references are dropped
    bool unowned = true;

    void start(float time = -1.0f);
    void update(float delta);
    void timeout();
    void stop();
};

struct TimerManager {
    vector<std::shared_ptr<Timer>> timers;
    vector<TimerCallback> timer_callbacks;
};

void update_timers(Scene* scene);

std::shared_ptr<Timer> add_timer(Scene* scene, std::function<void(Timer*)> callback = {}, bool unowned_oneshot = false);
std::shared_ptr<Timer> add_tween_timer(Scene* scene, std::function<void(Timer*)> callback = {}, bool unowned_oneshot = false);
void inspect(Timer* timer);

// To get rid of a timer, just drop references to it


}