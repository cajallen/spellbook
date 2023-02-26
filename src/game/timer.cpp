#include "timer.hpp"

#include <imgui.h>

#include "game/scene.hpp"

namespace spellbook {

Timer& add_timer(Scene* scene, const string& name, void(*callback)(Timer*, void*), void* payload, bool allocated_payload, bool permanent) {
    assert_else(!(callback != nullptr ^ payload != nullptr));
    auto& timer = *scene->timers.emplace(name);
    timer.scene = scene;
    timer.callback = TimerCallback{&timer, callback, payload, allocated_payload};
    timer.trigger_every_tick = false;
    timer.one_shot = !permanent;
    return timer;
}

Timer& add_tween_timer(Scene* scene, const string& name, void(*callback)(Timer*, void*), void* payload, bool allocated_payload, bool permanent) {
    assert_else(!(callback != nullptr ^ payload != nullptr));
    auto& timer = *scene->timers.emplace(name);
    timer.scene = scene;
    timer.callback = TimerCallback{&timer, callback, payload, allocated_payload};
    timer.trigger_every_tick = true;
    timer.one_shot = !permanent;
    return timer;
}

void destroy_timer(Timer* timer) {
    timer->stop();
    if (timer->callback.allocated_payload)
        free(timer->callback.payload);
}


void remove_timer(Scene* scene, const string& name) {
    for (auto it = scene->timers.begin(); it != scene->timers.end();) {
        if (it->name == name) {
            destroy_timer(&*it);
            it = scene->timers.erase(it);
        } else {
            ++it;
        }
    }
}

void remove_timer(Scene* scene, Timer* timer) {
    for (auto it = scene->timers.begin(); it != scene->timers.end();) {
        if (&*it == timer) {
            destroy_timer(timer);
            it = scene->timers.erase(it);
        } else {
            ++it;
        }
    }
}

void Timer::start(float new_time) {
    if (new_time != -1.0f)
        total_time = new_time;
    remaining_time = total_time;
    ticking = true;
}

void Timer::update(float delta) {
    if (!ticking)
        return;

    remaining_time -= delta;
    if (trigger_every_tick)
        if (callback.callback)
            scene->timer_callbacks.push_back(callback);
    if (remaining_time < 0.0f)
        timeout();
}

void Timer::timeout() {
    if (!trigger_every_tick && callback.callback)
        scene->timer_callbacks.push_back(callback);
    stop();
}

void Timer::stop() {
    remaining_time = 0.0f;
    ticking = false;
}

void inspect(Timer* timer) {
    if (timer->ticking)
        ImGui::SliderFloat("Timer", &timer->remaining_time, 0.0f, timer->total_time, "%.2f", ImGuiSliderFlags_NoInput);
    else
        ImGui::Text("Not ticking");
}

void update_timers(Scene* scene) {
    for (auto& timer : scene->timers) {
        timer.update(scene->delta_time);
    }
    for (auto& timer_callback : scene->timer_callbacks) {
        timer_callback.callback(timer_callback.timer, timer_callback.payload);
    }
    scene->timer_callbacks.clear();
    for (auto it = scene->timers.begin(); it != scene->timers.end();) {
        if (!it->ticking && it->one_shot) {
            destroy_timer(&*it);
            it = scene->timers.erase(it);
        }
        else
            ++it;
    }
}

}