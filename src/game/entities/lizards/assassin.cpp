#include "game/entities/lizards/lizard.hpp"

#include <entt/entity/entity.hpp>
#include <entt/core/hashed_string.hpp>

#include "extension/fmt.hpp"
#include "general/file/file_path.hpp"
#include "renderer/draw_functions.hpp"
#include "game/scene.hpp"
#include "game/pose_controller.hpp"
#include "game/entities/caster.hpp"
#include "game/entities/components.hpp"
#include "game/entities/targeting.hpp"
#include "game/entities/tags.hpp"
#include "game/entities/enemy.hpp"
#include "game/entities/lizards/lizard.hpp"

namespace spellbook {

using namespace entt::literals;

constexpr int poison_max_stacks = 4;
constexpr float poison_dps = 1.0f;
constexpr float poison_duration = 4.0f;
constexpr float contagion_update_delay = 0.5f;
constexpr float contagion_duration = 4.0f;
constexpr float untargetable_duration = 1.5f;

void handle_assassin_damaged(Scene* scene, entt::entity damager, entt::entity assassin, float damage_amount) {
    if (damage_amount <= 0.5f)
        return;
    Tags& tags = scene->registry.get<Tags>(assassin);
    tags.apply_tag("untargetable"_hs, "assassin_passive"_hs, assassin, untargetable_duration);

    LogicTransform& logic_tfm = scene->registry.get<LogicTransform>(assassin);
    EmitterCPU emitter_cpu = load_resource<EmitterCPU>("emitters/assassin/evasion.sbjemt"_resource);
    quick_emitter(scene, "Assassin evade", logic_tfm.position + v3(0.5f), emitter_cpu, untargetable_duration);
}

struct AssassinAttack : Attack {
    uint32 phase = 0;

    AssassinAttack(Scene* _scene, entt::entity _caster) : Attack(_scene, _caster) {
        auto& lizard_attack_entry = get_lizard_database().ability_entries.at(ability_index(LizardType_Assassin, AbilityType_Attack));
        setup_time(lizard_attack_entry.pre_time, lizard_attack_entry.post_time);
        setup_cd(lizard_attack_entry.extra_floats.contains("CD") ? lizard_attack_entry.extra_floats.at("CD") : 0.0f);
        Health& health = scene->registry.get<Health>(caster);
        entt::sink sink{health.damage_signal};
        sink.connect<handle_assassin_damaged>();

        entry_gather_function = gather_enemies();
        entry_eval_function = basic_lizard_entry_eval;
        set_anims = false;
    }
    ~AssassinAttack() override {
        if (!scene)
            return;
        Health* health = scene->registry.try_get<Health>(caster);
        if (health) {
            entt::sink sink{health->damage_signal};
            sink.disconnect<handle_assassin_damaged>();
        }
    }
    void start() override {
        auto poser = scene->registry.try_get<PoseController>(caster);
        if (poser) {
            poser->set_state(phase == 0 ? AnimationState_AttackInto : AnimationState_Attack2Into, pre_trigger_time.value());
        }
        lizard_turn_to_target();
    }

    void trigger() override {
        auto& logic_tfm = scene->registry.get<LogicTransform>(caster);
        v3 hit_vec = v3(target) - logic_tfm.position;

        uset<entt::entity> enemies = entry_gather_function(*this, target, 0.0f);
        for (auto& enemy : enemies) {
            Health& health = scene->registry.get<Health>(enemy);
            Tags& enemy_tags = scene->registry.get<Tags>(enemy);
            LogicTransform& enemy_tfm = scene->registry.get<LogicTransform>(enemy);
            health.damage(caster, 3.0f / enemies.size(), enemy_tfm.position - logic_tfm.position);

            StatEffect poison_effect = {StatEffect::Type_Add, poison_dps, poison_max_stacks, poison_duration};
            health.apply_dot(caster, "assassin_poison_ability"_hs, poison_effect, &load_resource<EmitterCPU>("emitters/assassin/poison.sbjemt"_resource));
            enemy_tags.apply_tag("assassin_poison"_hs, "assassin_attack"_hs, caster, poison_duration);
        }

        EmitterCPU emitter_cpu = load_resource<EmitterCPU>("emitters/assassin/basic_hit.sbjemt"_resource);
        emitter_cpu.rotation = math::quat_between(v3(1.0f, 0.0f, 0.0f), math::normalize(hit_vec));
        quick_emitter(scene, "Assassin hit", v3(target) + v3(0.5f), emitter_cpu, 0.2f);

        auto poser = scene->registry.try_get<PoseController>(caster);
        if (poser) {
            poser->set_state(phase == 0 ? AnimationState_AttackOut : AnimationState_Attack2Out, post_trigger_time.value());
        }

        phase = phase == 0 ? 1 : 0;
    }

    void end() override {
        if (phase == 0) {
            post_trigger_timer->stop();
            request_cast();
        } else {
            auto poser = scene->registry.try_get<PoseController>(caster);
            if (poser) {
                poser->set_state(AnimationState_Idle, post_trigger_time.value());
            }
        }
    }

    void targeting() override {
        Caster& caster_comp = scene->registry.get<Caster>(caster);

        if (taunted(*this, caster_comp))
            return;

        plus_targeting(1, *this, entry_gather_function, entry_eval_function);
    }
};


struct AssassinSpell : Spell {
    shared_ptr<Timer> contagion_update;
    float last_trigger;

    AssassinSpell(Scene* _scene, entt::entity _caster) : Spell(_scene, _caster) {
        auto& lizard_spell_entry = get_lizard_database().ability_entries.at(ability_index(LizardType_Assassin, AbilityType_Spell));
        setup_time(lizard_spell_entry.pre_time, lizard_spell_entry.post_time);
        entry_gather_function = gather_enemies();
        entry_eval_function = basic_lizard_entry_eval;
    }

    void trigger() override {
        last_trigger = contagion_duration;
        contagion_update = add_tween_timer(scene, [this](Timer* timer) {
            if (!timer->scene->registry.valid(caster))
                return;

            LogicTransform& logic_tfm = timer->scene->registry.get<LogicTransform>(caster);
            if (timer->remaining_time < last_trigger - contagion_update_delay) {
                // collect spreaders
                uset<v3i> spreader_squares;
                for (auto [entity, enemy, enemy_tfm, tags] : timer->scene->registry.view<Enemy, LogicTransform, Tags>().each()) {
                    if (!tags.has_tag("assassin_poison"_hs))
                        continue;

                    v3i enemy_square = math::round_cast(enemy_tfm.position);
                    int level_diff = enemy_square.z - math::round_cast(logic_tfm.position.z);
                    if (level_diff > 0)
                        continue;
                    spreader_squares.insert(enemy_square);
                }

                vector<v3i> spread_squares = {};
                for (auto [entity, enemy, enemy_tfm] : timer->scene->registry.view<Enemy, LogicTransform>().each()) {
                    v3i enemy_square = math::round_cast(enemy_tfm.position);

                    for (const v3i& spreader : spreader_squares) {
                        int level_diff = enemy_square.z - spreader.z;
                        if (level_diff > 0)
                            continue;

                        if (math::abs(spreader.x - enemy_square.x) + math::abs(spreader.y - enemy_square.y) > 1)
                            continue;

                        Health& health = timer->scene->registry.get<Health>(entity);
                        Tags& enemy_tags = timer->scene->registry.get<Tags>(entity);

                        spread_squares.push_back(spreader);
                        spread_squares.push_back(enemy_square);

                        StatEffect poison_effect = {StatEffect::Type_Add, poison_dps, poison_max_stacks, poison_duration};
                        health.apply_dot(caster, "assassin_poison_ability"_hs, poison_effect, &load_resource<EmitterCPU>("emitters/assassin/poison.sbjemt"_resource));
                        enemy_tags.apply_tag("assassin_poison"_hs, "assassin_ability"_hs, caster, poison_duration);
                    }
                }

                add_tween_timer(timer->scene, [spread_squares](Timer* timer) {
                    if (timer->ticking) {
                        float width = (timer->remaining_time / timer->total_time) * 0.04f;
                        timer->scene->render_scene.quick_mesh(generate_outline(
                                &timer->scene->camera,
                                timer->scene->map_data.solids,
                                spread_squares,
                                palette::yellow_green,
                                width
                            ), true, false
                        );
                    }
                }, true)->start(0.15f);

                last_trigger = timer->remaining_time;
            }
        });

        contagion_update->start(contagion_duration);
    }
};

template<>
void draw_dragging_preview<LizardType_Assassin>(Scene* scene, entt::entity entity) {
    v3 logic_pos = scene->registry.get<Dragging>(entity).potential_logic_position;
    v3i logic_posi = math::round_cast(logic_pos);
    
    vector<v3i> center_pieces = {logic_posi};
    for (const v2i& offset : {v2i{-1, 0}, v2i{0, -1}, v2i{1, 0}, v2i{0, 1}}) {
        center_pieces.push_back(logic_posi + v3i(offset, 0));
    }
    scene->render_scene.quick_mesh(
        generate_outline(&scene->camera, scene->map_data.solids, center_pieces, palette::green_yellow, 0.05f),
        true, false);
}

template<>
void build<LizardType_Assassin>(Scene* scene, entt::entity entity) {
    scene->registry.emplace<Lizard>(entity, LizardType_Assassin, v3::X, draw_dragging_preview<LizardType_Assassin>);
    Caster& caster = scene->registry.get<Caster>(entity);

    caster.attack = std::make_unique<AssassinAttack>(scene, entity);
    caster.spell = std::make_unique<AssassinSpell>(scene, entity);
}

}
