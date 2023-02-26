#include "game/entities/lizards/lizard_builder.hpp"

#include <entt/entity/entity.hpp>

#include "game/scene.hpp"
#include "game/pose_controller.hpp"
#include "game/entities/components.hpp"
#include "game/entities/lizard.hpp"

namespace spellbook {

void illusion_attack_start(void* payload) {
    auto ability = id_ptr<Ability>((u64) payload);
    auto poser = ability->scene->registry.try_get<PoseController>(ability->caster);
    if (poser) {
        poser->set_state(AnimationState_AttackInto, ability->pre_trigger_time.value());
    }
}

void illusion_attack_trigger(void* payload) {
    auto ability = id_ptr<Ability>((u64) payload);
    struct IllusionBasicHurtPayload {
        Scene* scene;
        Lizard* lizard;
        v3i target;
    };

    quick_emitter(ability->scene, "Illusion Basic", v3(ability->target), "emitters/shadow_daggers.sbemt", 0.2f);
    add_timer(ability->scene, "Illusion Basic Hurt Timer", [](Timer* timer, void* data) {
        auto payload = (IllusionBasicHurtPayload*) data;
        auto scene = payload->scene;
        auto enemies = scene->get_enemies(payload->target);
        for (auto& enemy : enemies) {
            auto& health = scene->registry.get<Health>(enemy);
            health.damage(2.5f, v3(0, 0, -1));
        }
    },  new IllusionBasicHurtPayload{ability->scene,
        &ability->scene->registry.get<Lizard>(ability->caster),
        ability->target
    }, true, false).start(0.3f);
    
    auto poser = ability->scene->registry.try_get<PoseController>(ability->caster);
    if (poser) {
        poser->set_state(AnimationState_AttackOut, ability->pre_trigger_time.value());
    }
}

void illusion_attack_targeting(void* payload) {
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
    for (int x = -3; x <= 3; x++) {
        for (int y = -3; y <= 3; y++) {
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

void build_illusion_mage(Scene* scene, entt::entity entity, const LizardPrefab& lizard_prefab) {
    auto& liz = scene->registry.emplace<Lizard>(entity, lizard_prefab.type, lizard_prefab.default_direction);
    
    liz.basic_ability = make_ability(scene, "Illusion Basic");
    liz.basic_ability->caster = entity;
    liz.basic_ability->pre_trigger_time = Stat(0.8f);
    liz.basic_ability->post_trigger_time = Stat(0.3f);
    liz.basic_ability->cooldown_time = Stat(0.3f);
    liz.basic_ability->start_callback = illusion_attack_start;
    liz.basic_ability->start_payload = (void*) liz.basic_ability.id;
    liz.basic_ability->trigger_callback = illusion_attack_trigger;
    liz.basic_ability->trigger_payload = (void*) liz.basic_ability.id;
    liz.basic_ability->targeting_callback = illusion_attack_targeting;
    liz.basic_ability->targeting_payload = (void*) liz.basic_ability.id;
}

}