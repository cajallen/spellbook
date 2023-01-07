#include "timer.hpp"

#include "game/scene.hpp"

namespace spellbook {

Timer& add_timer(Scene* scene, string name, void(*callback)(void*), void* payload, float time) {
    assert_else(!(callback != nullptr ^ payload != nullptr));
    return *scene->timers.emplace(name, time, TimerCallback{callback, payload}, scene);
}

void remove_timer(Scene* scene, string name) {
    scene->timers.erase(std::remove_if(scene->timers.begin(), scene->timers.end(),
        [&name](const Timer& timer) { return timer.name == name; }
    ));
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
    if (remaining_time < 0.0f)
        timeout();
}

void Timer::timeout() {
    stop();
    if (callback.callback)
        scene->timer_callbacks.push_back(callback);
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

}