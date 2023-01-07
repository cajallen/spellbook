#include "ability.hpp"

#include "extension/fmt.hpp"
#include "game/scene.hpp"

namespace spellbook {

id_ptr<Ability> make_ability(Scene* set_scene, const string& set_name) {
    auto ability = id_ptr<Ability>::emplace();
    ability->scene = set_scene;
    ability->name = set_name;
    ability->pre_trigger_timer = &add_timer(set_scene, fmt_("{}::pre", set_name),
        [](void* payload) {
            auto ability = id_ptr<Ability>((u64) payload);
            if (ability->trigger_callback != NULL)
                ability->trigger_callback(ability->trigger_payload);
            ability->post_trigger_timer->start(ability->post_trigger_time.value());
            ability->cooldown_timer->start(ability->cooldown_time.value());
        }, (void*) ability.id
    );
    ability->post_trigger_timer = &add_timer(set_scene, fmt_("{}::post", set_name),
    [](void* payload) {
            auto ability = id_ptr<Ability>((u64) payload);
            if (ability->end_callback != nullptr)
                ability->end_callback(ability->end_payload);
        }, (void*) ability.id
    );
    ability->cooldown_timer = &add_timer(set_scene, fmt_("{}::cd", set_name),
        [](void* payload) {
            auto ability = id_ptr<Ability>((u64) payload);
            if (ability->ready_callback != nullptr)
                ability->ready_callback(ability->ready_payload);
        }, (void*) ability.id
    );
    return ability;
}

void Ability::request_cast() {
    if (start_callback != nullptr)
        start_callback(start_payload);
    pre_trigger_timer->start(pre_trigger_time.value());
}

void inspect(Ability* ability) {
    ImGui::PushID(ability);
    
    if (ability->has_target) {
        ImGui::DragInt3("Target", ability->target.data, 0.01f, 0, 0, "%d", ImGuiSliderFlags_NoInput);
    } else {
        ImGui::Text("No target");
    }
    
    if (ImGui::TreeNode("Pre Trigger Time")) {
        inspect(ability->pre_trigger_timer);
        inspect(&ability->pre_trigger_time);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Post Trigger Time")) {
        inspect(ability->post_trigger_timer);
        inspect(&ability->post_trigger_time);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Cooldown Time")) {
        inspect(ability->cooldown_timer);
        inspect(&ability->cooldown_time);
        ImGui::TreePop();
    }
    ImGui::PopID();
}

}