#include "game/entities/lizards/lizard.hpp"

#include <entt/entity/entity.hpp>

#include "general/math/matrix_math.hpp"
#include "renderer/draw_functions.hpp"
#include "game/scene.hpp"
#include "game/pose_controller.hpp"
#include "game/entities/caster.hpp"
#include "game/entities/components.hpp"
#include "game/entities/enemy.hpp"
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
    lizard_turn_to_target();

    v3 pos_cap = logic_tfm.position;
    add_timer(scene, "taunt", [pos_cap](Timer* timer) {
        timer->scene->audio.play_sound("audio/champion/axe_fly.wav"_resource, {.position = pos_cap});
    }, true)->start(0.1f);
}

void ChampionAttack::trigger() {
    LogicTransform& logic_tfm = scene->registry.get<LogicTransform>(caster);
    scene->audio.play_sound("audio/champion/axe_hit.wav"_resource, {.position = logic_tfm.position});

    entt::entity caster_cap = caster;
    for (auto& enemy : entry_gather_function(*this, target, 0.0f)) {
        entt::entity enemy_attachment = scene->registry.get<Enemy>(enemy).attachment;
        Caster* enemy_caster = scene->registry.try_get<Caster>(enemy_attachment);
        LogicTransform& enemy_tfm = scene->registry.get<LogicTransform>(enemy);
        Health& health = scene->registry.get<Health>(enemy);
        health.damage(caster, 2.0f, enemy_tfm.position - logic_tfm.position);

        if (enemy_caster) {
            enemy_caster->taunt.set(uint64(this), caster);
            
            add_timer(scene, "taunt", [enemy_attachment, caster_cap](Timer* timer) {
                if (!timer->scene->registry.valid(caster_cap))
                    return;
                if (!timer->scene->registry.valid(enemy_attachment))
                    return;
                
                auto& enemy_caster = timer->scene->registry.get<Caster>(enemy_attachment);
                enemy_caster.taunt.reset(uint64(caster_cap) + 1);
            }, true)->start(2.0f);
        }
    }

    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            bool blocked = scene->map_data.solids.get(target + v3i(x, y, 0));
            bool has_floor = scene->map_data.solids.get(target + v3i(x, y, -1));

            if (!blocked && has_floor) {
                quick_emitter(scene, "Champion Basic", v3(target + v3i(x, y, 0)), "emitters/champion/basic_hit.sbjemt"_resource, 0.20f);
            } else if (false) {
                quick_emitter(scene, "Champion Basic", v3(target + v3i(x, y, 0)), "emitters/champion/basic_miss.sbjemt"_resource, 0.20f);
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
    
    ChampionSpell(Scene* init_scene, entt::entity init_caster, float pre, float post);
    ~ChampionSpell();
    void targeting() override;
    void start() override;
    void trigger() override;

    string get_name() const override { return "Champion Spell"; }

    constexpr uint64 get_buff_id() { return hash_view("Champion Spell Resist"); }
    uint64 get_taunt_id() { return uint64(caster) + 1; }
};

void handle_champion_damaged(Scene* scene, entt::entity damager, entt::entity champion, float damage_amount) {
    ChampionSpell& champion_spell = (ChampionSpell&) *scene->registry.get<Caster>(champion).spell;
    champion_spell.damage_amount += damage_amount;
}
ChampionSpell::ChampionSpell(Scene* init_scene, entt::entity init_caster, float pre, float post) : Spell::Spell(init_scene, init_caster, pre, post) {
    Health& health = scene->registry.get<Health>(caster);
    entt::sink sink{health.damage_signal};
    sink.connect<handle_champion_damaged>();
}

ChampionSpell::~ChampionSpell() {
    Health* health = scene->registry.try_get<Health>(caster);
    if (!health)
        return;
    entt::sink sink{health->damage_signal};
    sink.disconnect<handle_champion_damaged>();
}

void ChampionSpell::start() {
    damage_amount = 0.0f;
}



void ChampionSpell::trigger() {
    // now
    Health& health = scene->registry.get<Health>(caster);

    uint64 buff_id = get_buff_id();
    
    health.damage_taken_multiplier->add_effect(buff_id, StatEffect{
        .type = StatEffect::Type_Multiply,
        .value = -0.9f
    });
    auto& emitters = scene->registry.get<EmitterComponent>(caster);
    emitters.add_emitter(buff_id, load_resource<EmitterCPU>("emitters/champion/spell_buff.sbjemt"_resource));
    
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
        EmitterCPU trigger_emitter = load_resource<EmitterCPU>("emitters/champion/spell_trigger.sbjemt"_resource);
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
            Health& health = timer->scene->registry.get<Health>(enemy);
            health.damage(caster_cap, spell.damage_amount, enemy_tfm.position - logic_tfm.position);
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
                enemy_caster->taunt.set(uint64(caster_cap) + 1, caster_cap);
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
    
    vector<v3i> center_pieces = {logic_posi};
    vector<v3i> aoe_pieces = {};
    for (const v2i& offset : {v2i{-1, 0}, v2i{0, -1}, v2i{1, 0}, v2i{0, 1}}) {
        center_pieces.push_back(logic_posi + v3i(offset, 0));
        if (scene->map_data.solids.get(logic_posi + v3i(offset, -1)) && !scene->map_data.solids.get(logic_posi + v3i(offset, 0))) {
            v3i it = v3i(-1, -1, 0);
            do {
                aoe_pieces.push_back(logic_posi + v3i(offset, 0) + it);
            } while (math::iterate(it, v3i(-1, -1, 0), v3i(1, 1, 0)));
        }
    }
    scene->render_scene.quick_mesh(
        generate_outline(&scene->camera, scene->map_data.solids, center_pieces, palette::aquamarine, 0.05f),
        true, false);
    scene->render_scene.quick_mesh(
        generate_outline(&scene->camera, scene->map_data.solids, aoe_pieces, palette::sea_green, 0.03f),
        true, false);
}

}
