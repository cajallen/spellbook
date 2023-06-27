﻿#include "game/entities/lizards/lizard_builder.hpp"

#include <entt/entity/entity.hpp>

#include "general/matrix_math.hpp"
#include "renderer/draw_functions.hpp"
#include "game/scene.hpp"
#include "game/pose_controller.hpp"
#include "game/entities/caster.hpp"
#include "game/entities/components.hpp"
#include "game/entities/enemy.hpp"
#include "game/entities/impair.hpp"
#include "game/entities/lizard.hpp"
#include "game/entities/targeting.hpp"

namespace spellbook {

struct ChampionAttack : Attack {
    using Attack::Attack;
    void targeting() override;
    void start() override;
    void trigger() override;

    string get_name() const override { return "Champion Attack"; }
};

void ChampionAttack::start() {
    LogicTransform& logic_tfm = scene->registry.get<LogicTransform>(caster);
    v3i caster_pos = math::round_cast(logic_tfm.position);
    auto lizard = scene->registry.try_get<Lizard>(caster);
    if (lizard) {
        v3 dir_to = math::normalize(v3(target) - v3(caster_pos));
        float ang = math::angle_difference(lizard->default_direction.xy, dir_to.xy);
        scene->registry.get<LogicTransform>(caster).rotation.yaw = ang;
    }

    v3 pos_cap = logic_tfm.position;
    add_timer(scene, "taunt", [pos_cap](Timer* timer) {
        timer->scene->audio.play_sound("audio/champion/axe_fly.wav", {.position = pos_cap});
    }, true)->start(0.1f);
}

void ChampionAttack::trigger() {
    LogicTransform& logic_tfm = scene->registry.get<LogicTransform>(caster);
    scene->audio.play_sound("audio/champion/axe_hit.wav", {.position = logic_tfm.position});

    entt::entity caster_cap = caster;
    for (auto& enemy : entry_gather_function(*this, target, 0.0f)) {
        entt::entity enemy_attachment = scene->registry.get<Enemy>(enemy).attachment;
        Caster* enemy_caster = scene->registry.try_get<Caster>(enemy_attachment);
        auto& logic_tfm = scene->registry.get<LogicTransform>(caster);
        auto& enemy_tfm = scene->registry.get<LogicTransform>(enemy);
        damage(scene, caster, enemy, 2.0f, enemy_tfm.position - logic_tfm.position);

        if (enemy_caster) {
            enemy_caster->taunt.set(u64(this), caster);
            
            add_timer(scene, "taunt", [enemy_attachment, caster_cap](Timer* timer) {
                if (!timer->scene->registry.valid(caster_cap))
                    return;
                if (!timer->scene->registry.valid(enemy_attachment))
                    return;
                
                auto& enemy_caster = timer->scene->registry.get<Caster>(enemy_attachment);
                enemy_caster.taunt.reset(u64(caster_cap) + 1);
            }, true)->start(2.0f);
        }
    }

    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            entt::entity tile = scene->get_tile(target + v3i(x, y, -1));
            if (tile == entt::null) {
                quick_emitter(scene, "Champion Basic", v3(target + v3i(x, y, 0)), "emitters/champion_basic_fizzle.sbemt", 0.20f);
                continue;
            }

            auto& grid_slot = scene->registry.get<GridSlot>(tile);
            if ((grid_slot.path || grid_slot.ramp) && x == 0 && y == 0) {
                quick_emitter(scene, "Champion Basic", v3(target + v3i(x, y, 0)), "emitters/champion_basic_hit.sbemt", 0.20f);
            } else {
                quick_emitter(scene, "Champion Basic", v3(target + v3i(x, y, 0)), "emitters/champion_basic_miss.sbemt", 0.20f);
            }
        }
    }
}

void ChampionAttack::targeting() {
    Caster& caster_comp = scene->registry.get<Caster>(caster);
    
    if (taunted(*this, caster_comp))
        return;

    if (plus_targeting(1, *this, entry_gather_function, entry_eval_function))
        return;
}

struct ChampionSpell : Spell {
    float damage_amount = 0.0f;
    std::shared_ptr<Timer> periodic_taunt_timer;
    
    using Spell::Spell;
    void targeting() override;
    void start() override;
    void trigger() override;

    string get_name() const override { return "Champion Spell"; }

    u64 get_buff_id() { return hash_string("Champion Spell Resist"); }
    u64 get_taunt_id() { return u64(caster) + 1; }
};

void ChampionSpell::start() {
    damage_amount = 0.0f;
}

void handle_champion_damaged(Scene* scene, entt::entity damager, entt::entity champion, float damage_amount) {
    ChampionSpell& champion_spell = (ChampionSpell&) *scene->registry.get<Caster>(champion).spell;
    champion_spell.damage_amount += damage_amount;
}


void ChampionSpell::trigger() {
    // now
    Health& health = scene->registry.get<Health>(caster);
    entt::sink sink{health.damage_signal};
    sink.connect<handle_champion_damaged>();

    u64 buff_id = get_buff_id();
    
    health.damage_taken_multiplier->add_effect(buff_id, StatEffect{
        .type = StatEffect::Type_Multiply,
        .value = -0.9f
    });
    auto& emitters = scene->registry.get<EmitterComponent>(caster);
    emitters.add_emitter(buff_id, load_asset<EmitterCPU>("emitters/champion/spell_buff.sbemt"));
    
    entt::entity caster_cap = caster;

    // after 4 seconds
    add_timer(scene, "champion_spell end", [caster_cap, buff_id](Timer* timer) {
        if (!timer->scene->registry.valid(caster_cap))
            return;

        // reset buff
        Health& health = timer->scene->registry.get<Health>(caster_cap);

        health.damage_taken_multiplier->remove_effect(buff_id);
        auto& emitters = timer->scene->registry.get<EmitterComponent>(caster_cap);
        emitters.remove_emitter(buff_id);

        // emit effect
        LogicTransform& logic_tfm = timer->scene->registry.get<LogicTransform>(caster_cap);
        EmitterCPU trigger_emitter = load_asset<EmitterCPU>("emitters/champion/spell_trigger.sbemt");
        quick_emitter(timer->scene, "Champion spell trigger", logic_tfm.position + v3(0.5f), trigger_emitter, 0.2f);

        ChampionSpell& spell = (ChampionSpell&) *timer->scene->registry.get<Caster>(caster_cap).spell;
        spell.periodic_taunt_timer.reset();

        // reset taunts
        for (auto [entity, enemy] : timer->scene->registry.view<Enemy>().each()) {
            Caster* enemy_caster = timer->scene->registry.try_get<Caster>(enemy.attachment);
            if (enemy_caster)
                enemy_caster->taunt.reset(spell.get_taunt_id());
        }

        // do damage
        for (auto& enemy : spell.entry_gather_function(spell, spell.target, 0.0f)) {
            LogicTransform& enemy_tfm = timer->scene->registry.get<LogicTransform>(enemy);
            damage(timer->scene, caster_cap, enemy, spell.damage_amount, enemy_tfm.position - logic_tfm.position);
        }

        spell.damage_amount = 0.0f;
    }, true)->start(4.0f);

    // periodically during the 4 seconds
    int phase = 0;
    constexpr float period = 0.5f;
    periodic_taunt_timer = add_timer(scene, "taunt", [&phase, caster_cap, period](Timer* timer) {
        ChampionSpell& spell = (ChampionSpell&) *timer->scene->registry.get<Caster>(caster_cap).spell;
        for (auto& enemy : spell.entry_gather_function(spell, spell.target, 0.0f)) {
            entt::entity enemy_attachment = timer->scene->registry.get<Enemy>(enemy).attachment;
            Caster* enemy_caster = timer->scene->registry.try_get<Caster>(enemy_attachment);

            if (enemy_caster)
                enemy_caster->taunt.set(u64(caster_cap) + 1, caster_cap);
        }
        phase++;
        if (phase < math::ceil_cast(4.0f / period)) {
            timer->start(period);
        }
    }, false);
    periodic_taunt_timer->start(period);
}



void ChampionSpell::targeting() {
    LogicTransform& logic_tfm = scene->registry.get<LogicTransform>(caster);
    target = math::round_cast(logic_tfm.position);
    has_target = true;
}

void build_champion(Scene* scene, entt::entity entity, const LizardPrefab& lizard_prefab) {
    scene->registry.emplace<Lizard>(entity, lizard_prefab.type, lizard_prefab.default_direction);
    Caster& caster = scene->registry.get<Caster>(entity);

    caster.attack = std::make_unique<ChampionAttack>(scene, entity, 0.8f, 1.3f, 1.0f, 3.0f);
    caster.attack->entry_gather_function = gather_enemies_aoe(1);
    caster.attack->entry_eval_function = basic_lizard_entry_eval;

    caster.spell = std::make_unique<ChampionSpell>(scene, entity, 0.2f, 4.0f);
    caster.spell->entry_gather_function = gather_enemies_floor();
    caster.spell->entry_eval_function = basic_lizard_entry_eval;
}

void draw_champion_dragging_preview(Scene* scene, entt::entity entity) {
    v3 logic_pos = scene->registry.get<Dragging>(entity).potential_logic_position;
    v3i logic_posi = math::round_cast(logic_pos);
    
    vector<FormattedVertex> vertices;
    for (const v2i& offset : {v2i{-1, 0}, v2i{0, -1}, v2i{1, 0}, v2i{0, 1}}) {
        if (scene->get_tile(logic_posi + v3i(offset.x, offset.y, -1)) != entt::null) {
            v3 pos = logic_pos + v3(0.5f, 0.5f, 0.02f) + v3(offset.x, offset.y, 0.0f);
            add_formatted_square(vertices, pos, v3(0.5f, 0.f, 0.f), v3(0.f, 0.5f, 0.f), palette::aquamarine, 0.05f);
            add_formatted_square(vertices, pos, v3(1.5f, 0.f, 0.f), v3(0.f, 1.5f, 0.f), palette::dark_sea_green, 0.03f);
        }
    }
    if (vertices.empty())
        return;
    scene->render_scene.quick_mesh(generate_formatted_line(&scene->camera, vertices), true, false);
}

}
