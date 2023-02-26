#include "game/entities/lizards/lizard_builder.hpp"

#include <entt/entity/entity.hpp>

#include "general/matrix_math.hpp"
#include "game/game.hpp"
#include "game/scene.hpp"
#include "game/pose_controller.hpp"
#include "game/entities/components.hpp"
#include "game/entities/lizard.hpp"

namespace spellbook {

void warlock_attack_start(void* payload) {
    auto ability = id_ptr<Ability>((u64) payload);

    add_tween_timer(ability->scene, "Emissive Up", [](Timer* timer, void* payload) {
        auto ribbon_mat = game.renderer.get_material("models\\liz\\Ribbon.sbmat");
        auto glyph_mat = game.renderer.get_material("models\\liz\\gylph.sbmat");
        ribbon_mat->tints.emissive_tint.a = 1.0f - timer->remaining_time / timer->total_time;
        glyph_mat->tints.emissive_tint.a = 1.0f - timer->remaining_time / timer->total_time;
    }, (void*) ability.id, false, false).start(ability->pre_trigger_time.value());
}

void warlock_attack_trigger(void* payload) {
    auto ability = id_ptr<Ability>((u64) payload);
    auto model = ability->scene->registry.try_get<Model>(ability->caster);
    auto transform = ability->scene->registry.try_get<ModelTransform>(ability->caster);
    auto& skeleton = model->model_cpu->skeleton;
    for (auto& bone : skeleton->bones) {
        if (bone->name == "crystal") {
            m44 t =  transform->get_transform() * model->model_cpu->root_node->cached_transform * bone->final_transform();
            v3 pos = math::apply_transform(t, v3(0.0f, 0.5f, 0.0f));
            quick_emitter(ability->scene, "Warlock Crush", v3(pos), "emitters/warlock_crush.sbemt", 0.1f);
        }
    }
    add_tween_timer(ability->scene, "Emissive Down", [](Timer* timer, void* payload) {
        auto ribbon_mat = game.renderer.get_material("models\\liz\\Ribbon.sbmat");
        auto glyph_mat = game.renderer.get_material("models\\liz\\gylph.sbmat");
        ribbon_mat->tints.emissive_tint.a = timer->remaining_time / timer->total_time;
        glyph_mat->tints.emissive_tint.a = timer->remaining_time / timer->total_time;
    }, (void*) ability.id, false, false).start(ability->post_trigger_time.value());
}

void warlock_attack_targeting(void* payload) {
    auto ability = id_ptr<Ability>((u64) payload);
    v3i caster_pos = math::round_cast(ability->scene->registry.get<LogicTransform>(ability->caster).position);
    struct Entry {
        v3i offset = {};
        int count;
    };
    vector<Entry> entries;
    auto add_entry = [&ability, &entries, &caster_pos](v3i offset) {
        entries.emplace_back(offset, ability->scene->get_enemies(caster_pos + offset).size());
    };
    for (int x = -2; x <= 2; x++) {
        for (int y = -2; y <= 2; y++) {
            add_entry(v3i(x,y,0));
        }
    }
    vector closest_entries = {entries.front()};
    for (auto& entry : entries) {
        if (entry.count > closest_entries.begin()->count)
            closest_entries = {entry};
        else if (entry.count == closest_entries.begin()->count)
            closest_entries.push_back(entry);
    }
    if (closest_entries.front().count > 0) {
        ability->target = caster_pos + closest_entries[math::random_s32(closest_entries.size())].offset;
        ability->has_target = true;
    } else {
        ability->has_target = false;
    }
}


void build_warlock(Scene* scene, entt::entity entity, const LizardPrefab& lizard_prefab) {
    auto& liz = scene->registry.emplace<Lizard>(entity, lizard_prefab.type, lizard_prefab.default_direction);
    
    liz.basic_ability = make_ability(scene, "Warlock Basic");
    liz.basic_ability->caster = entity;
    liz.basic_ability->pre_trigger_time = Stat(0.5f);
    liz.basic_ability->post_trigger_time = Stat(1.0f);
    liz.basic_ability->cooldown_time = Stat(1.2f);
    liz.basic_ability->start_callback = warlock_attack_start;
    liz.basic_ability->start_payload = (void*) liz.basic_ability.id;
    liz.basic_ability->trigger_callback = warlock_attack_trigger;
    liz.basic_ability->trigger_payload = (void*) liz.basic_ability.id;
    liz.basic_ability->targeting_callback = warlock_attack_targeting;
    liz.basic_ability->targeting_payload = (void*) liz.basic_ability.id;
}

}