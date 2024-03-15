#include "game/entities/lizards/lizard.hpp"

#include <entt/entity/entity.hpp>

#include "general/math/matrix_math.hpp"
#include "game/scene.hpp"
#include "game/pose_controller.hpp"
#include "game/entities/caster.hpp"
#include "game/entities/components.hpp"
#include "game/entities/lizard.hpp"
#include "game/entities/projectile.hpp"
#include "game/entities/targeting.hpp"
#include "renderer/draw_functions.hpp"

namespace spellbook {

const float base_projectile_speed = 4.0f;

struct WarlockAttack : Attack {
    using Attack::Attack;
    void targeting() override;
    void start() override;
    void trigger() override;
    float time_to_hit(v3i pos) override;
    string get_name() const override { return "Warlock Spell"; }
};

void WarlockAttack::start() {
    lizard_turn_to_target();
}

void WarlockAttack::trigger() {
    auto& warlock_l_transform = scene->registry.get<LogicTransform>(caster);
    
    Caster& caster_comp = scene->registry.get<Caster>(caster);
    Model* model = scene->registry.try_get<Model>(caster);
    ModelTransform* transform = scene->registry.try_get<ModelTransform>(caster);
    std::unique_ptr<SkeletonCPU>& skeleton = model->model_cpu->skeleton;

    v3 pot_pos = warlock_l_transform.position;
    v3 pot_dir = math::rotate(transform->rotation, v3(1.0f, 0.0f, 0.0f));
    Bone* pot_bone = skeleton->find_bone("Pot");
    if (pot_bone) {
        m44 t =  transform->get_transform() * model->model_cpu->root_node->cached_transform * pot_bone->transform();
        pot_pos = math::apply_transform(t, v3(0.0f, 0.2f, 0.0f));
        v3 end_vec = math::apply_transform(t, v3(0.0f, 1.0f, 0.0f));
        pot_dir = math::normalize(end_vec - pot_pos);
        pot_pos -= v3(0.5f);
    }
    EmitterCPU emitter_cpu = load_resource<EmitterCPU>("emitters/warlock/pot_spray.sbjemt"_resource);
    emitter_cpu.rotation = math::quat_between(v3(1.0f, 0.0f, 0.0f), pot_dir);
    quick_emitter(scene, "Warlock Pot Spray", pot_pos + v3(0.5f), emitter_cpu, 0.3f);
    
    Projectile projectile = {
        .target = target,
        .speed = StatInstance{&*caster_comp.projectile_speed, base_projectile_speed},
        .callback = [this](entt::entity proj_entity) {
            Projectile* projectile = scene->registry.try_get<Projectile>(proj_entity);
            if (!projectile)
                return;
            LogicTransform& logic_tfm = scene->registry.get<LogicTransform>(caster);
            EmitterCPU hit_emitter = load_resource<EmitterCPU>("emitters/warlock/basic_hit.sbjemt"_resource);
            hit_emitter.set_velocity_direction(math::normalize(v3(projectile->target) - logic_tfm.position));
            quick_emitter(scene, "Warlock Basic Hit", v3(projectile->target) + v3(0.5f), hit_emitter, 0.1f);
            for (entt::entity enemy : entry_gather_function(*this, projectile->target, 0.0f)) {
                Health& health = scene->registry.get<Health>(enemy);
                LogicTransform& enemy_tfm = scene->registry.get<LogicTransform>(enemy);
                v3 damage_dir = v3(0.0f);
                damage_dir = enemy_tfm.position - logic_tfm.position;
                health.damage(caster, 2.0f, damage_dir);
            }
        }
    };
    quick_projectile(scene, projectile, pot_pos, "emitters/warlock/basic_proj.sbjemt"_resource);
}

void WarlockAttack::targeting() {
    Caster& caster_comp = scene->registry.get<Caster>(caster);
    
    if (taunted(*this, caster_comp))
        return;

    if (square_targeting(2, *this, entry_gather_function, entry_eval_function))
        return;
}

float WarlockAttack::time_to_hit(v3i pos) {
    auto& l_transform = scene->registry.get<LogicTransform>(caster);
    Caster& caster_comp = scene->registry.get<Caster>(caster);
    float travel_time = math::distance(l_transform.position, v3(pos)) / stat_instance_value(&*caster_comp.projectile_speed, base_projectile_speed);
    return pre_trigger_time.value() + travel_time;
}


struct WarlockSpell : Spell {
    using Spell::Spell;

    void trigger() override;
    string get_name() const override { return "Warlock Spell"; }
};

void WarlockSpell::trigger() {
    constexpr float buff_duration = 12.0f;

    auto& warlock_logic_tfm = scene->registry.get<LogicTransform>(caster);
    
    auto model = scene->registry.try_get<Model>(caster);
    auto transform = scene->registry.try_get<ModelTransform>(caster);
    auto& skeleton = model->model_cpu->skeleton;

    v3 pot_pos = warlock_logic_tfm.position;
    v3 pot_dir = v3(math::cos(warlock_logic_tfm.yaw), math::sin(warlock_logic_tfm.yaw), 0.0f);
    Bone* pot_bone = skeleton->find_bone("Pot");
    if (pot_bone) {
        m44 t =  transform->get_transform() * model->model_cpu->root_node->cached_transform * pot_bone->transform();
        pot_pos = math::apply_transform(t, v3(0.0f, 0.2f, 0.0f));
        v3 end_vec = math::apply_transform(t, v3(0.0f, 1.0f, 0.0f));
        pot_dir = math::normalize(end_vec - pot_pos);
        pot_pos -= v3(0.5f);
    }
    EmitterCPU emitter_cpu = load_resource<EmitterCPU>("emitters/warlock/ability_pot_spray.sbjemt"_resource);
    emitter_cpu.rotation = math::quat_between(v3(1.0f, 0.0f, 0.0f), pot_dir);
    quick_emitter(scene, "Warlock Ability Pot Spray", pot_pos + v3(0.5f), emitter_cpu, 0.3f);
    quick_emitter(scene, "Warlock Aura Burst", warlock_logic_tfm.position + v3(0.5f, 0.5f, 0.0f), load_resource<EmitterCPU>("emitters/warlock/ability_base.sbjemt"_resource), 0.2f);
    quick_emitter(scene, "Warlock Aura Ticking", warlock_logic_tfm.position + v3(0.5f), load_resource<EmitterCPU>("emitters/warlock/ability_ticking.sbjemt"_resource), buff_duration);

    Caster& caster_comp = scene->registry.get<Caster>(caster);
    caster_comp.attack_speed->add_effect((uint64) this, StatEffect(StatEffect::Type_Multiply, -0.4f, INT_MAX, buff_duration));
    caster_comp.cooldown_speed->add_effect((uint64) this, StatEffect(StatEffect::Type_Multiply, -0.3f, INT_MAX, buff_duration));
    caster_comp.lifesteal->add_effect((uint64) this, StatEffect(StatEffect::Type_Base, 0.1f, INT_MAX, buff_duration));
}


void build_warlock(Scene* scene, entt::entity entity, const LizardPrefab& lizard_prefab) {
    scene->registry.emplace<Lizard>(entity, lizard_prefab.type, lizard_prefab.default_direction);
    Caster& caster = scene->registry.get<Caster>(entity);

    caster.attack = std::make_unique<WarlockAttack>(scene, entity, 1.0f, 1.0f, 1.0f, 3.0f);
    caster.attack->entry_gather_function = gather_enemies();
    caster.attack->entry_eval_function = basic_lizard_entry_eval;


    caster.spell = std::make_unique<WarlockSpell>(scene, entity, 0.8f, 0.6f);
    caster.spell->entry_gather_function = gather_enemies();
    caster.spell->entry_eval_function = basic_lizard_entry_eval;
}

void draw_warlock_dragging_preview(Scene* scene, entt::entity entity) {
    v3 logic_pos = scene->registry.get<Dragging>(entity).potential_logic_position;
    
    vector<FormattedVertex> vertices;
    v3 pos = logic_pos + v3(0.5f, 0.5f, 0.02f);
    add_formatted_square(vertices, pos, v3(2.5f, 0.f, 0.f), v3(0.f, 2.5f, 0.f), palette::indian_red, 0.05f);
    if (vertices.empty())
        return;
    scene->render_scene.quick_mesh(generate_formatted_line(&scene->camera, vertices), true, false);
}

}
