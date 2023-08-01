#include "timer.hpp"

#include <imgui.h>

#include "game/scene.hpp"

namespace spellbook {


std::shared_ptr<Timer> add_timer(Scene* scene, const string& name, std::function<void(Timer*)> callback, bool unowned_oneshot) {
    std::shared_ptr<Timer> timer_ptr = std::make_shared<Timer>(name, scene);
    scene->timer_manager.timers.push_back(timer_ptr);
    Timer& timer = *timer_ptr;
    timer.callback = TimerCallback{&timer, std::move(callback)};
    timer.trigger_every_tick = false;
    timer.unowned = unowned_oneshot;
    return timer_ptr;
}

std::shared_ptr<Timer> add_tween_timer(Scene* scene, const string& name, std::function<void(Timer*)> callback, bool unowned_oneshot) {
    std::shared_ptr<Timer> timer_ptr = std::make_shared<Timer>(name, scene);
    scene->timer_manager.timers.push_back(timer_ptr);
    Timer& timer = *timer_ptr;
    timer.callback = TimerCallback{&timer, callback};
    timer.trigger_every_tick = true;
    timer.unowned = unowned_oneshot;
    return timer_ptr;
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
            scene->timer_manager.timer_callbacks.push_back(callback);
    if (remaining_time < 0.0f)
        timeout();
}

void Timer::timeout() {
    if (!trigger_every_tick && callback.callback)
        scene->timer_manager.timer_callbacks.push_back(callback);
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
    scene->timer_manager.timers.remove_if([](const shared_ptr<Timer>& it) -> bool {
        bool unowned_and_done = it->unowned && !it->ticking;
        bool owned_and_dead = !it->unowned && it.use_count() == 1;
        return unowned_and_done || owned_and_dead;
    }, true);
    for (auto timer : scene->timer_manager.timers) {
        timer->update(scene->delta_time);
    }
    for (auto& timer_callback : scene->timer_manager.timer_callbacks) {
        timer_callback.callback(timer_callback.timer);
    }
    scene->timer_manager.timer_callbacks.clear();
    scene->timer_manager.timers.remove_if([](const shared_ptr<Timer>& it) -> bool {
        bool unowned_and_done = it->unowned && !it->ticking;
        bool owned_and_dead = !it->unowned && it.use_count() == 1;
        return unowned_and_done || owned_and_dead;
    }, true);
}

}