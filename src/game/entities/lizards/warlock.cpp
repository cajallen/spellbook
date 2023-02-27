#include "game/entities/lizards/lizard_builder.hpp"

#include <entt/entity/entity.hpp>

#include "general/matrix_math.hpp"
#include "game/game.hpp"
#include "game/scene.hpp"
#include "game/pose_controller.hpp"
#include "game/entities/components.hpp"
#include "game/entities/lizard.hpp"
#include "game/entities/projectile.hpp"
#include "renderer/draw_functions.hpp"

namespace spellbook {

void warlock_attack_start(void* payload) {
    auto ability_ptr = id_ptr<Ability>((u64) payload);
    Ability& ability = *ability_ptr;

    auto poser = ability.scene->registry.try_get<PoseController>(ability.caster);
    if (poser) {
        poser->set_state(AnimationState_AttackInto, ability.pre_trigger_time.value());
    }

    v3i caster_pos = math::round_cast(ability.scene->registry.get<LogicTransform>(ability.caster).position);
    auto lizard = ability.scene->registry.try_get<Lizard>(ability.caster);
    if (lizard) {
        v3 dir_to = math::normalize(v3(ability.target) - v3(caster_pos));
        float ang = math::angle_difference(lizard->default_direction.xy, dir_to.xy);
        ability.scene->registry.get<LogicTransform>(ability.caster).rotation.yaw = ang;
    }
}

void warlock_attack_trigger(void* payload) {
    auto ability_ptr = id_ptr<Ability>((u64) payload);
    Ability& ability = *ability_ptr;
    auto& warlock_l_transform = ability.scene->registry.get<LogicTransform>(ability.caster);
    
    auto model = ability.scene->registry.try_get<Model>(ability.caster);
    auto transform = ability.scene->registry.try_get<ModelTransform>(ability.caster);
    auto& skeleton = model->model_cpu->skeleton;


    v3 pot_pos = warlock_l_transform.position;
    v3 pot_dir = v3(math::cos(transform->rotation.yaw), math::sin(transform->rotation.yaw), 0.0f);
    for (auto& bone : skeleton->bones) {
        if (bone->name == "Pot") {
            m44 t =  transform->get_transform() * model->model_cpu->root_node->cached_transform * bone->transform();
            pot_pos = math::apply_transform(t, v3(0.0f, 0.2f, 0.0f));
            v3 end_vec = math::apply_transform(t, v3(0.0f, 1.0f, 0.0f));
            pot_dir = math::normalize(end_vec - pot_pos);
            pot_pos -= v3(0.5f);
        }
    }
    EmitterCPU emitter_cpu = load_asset<EmitterCPU>("emitters/warlock/pot_spray.sbemt");
    emitter_cpu.velocity = math::length(emitter_cpu.velocity) * pot_dir;
    quick_emitter(ability.scene, "Warlock Pot Spray", pot_pos + v3(0.5f), emitter_cpu, 0.3f);

    struct WarlockProjectilePayload {
        Scene* scene;
        entt::entity caster;
    };
    
    Projectile projectile = {
        .target = ability.target,
        .speed = Stat(4.0f),
        .callback = [](entt::entity proj_entity, void* data) {
            auto payload = (WarlockProjectilePayload*) data;
            auto projectile = payload->scene->registry.try_get<Projectile>(proj_entity);
            if (projectile) {
                auto this_lt = payload->scene->registry.try_get<LogicTransform>(payload->caster);
                EmitterCPU hit_emitter = load_asset<EmitterCPU>("emitters/warlock/basic_hit.sbemt");
                hit_emitter.velocity = math::length(hit_emitter.velocity) * math::normalize(v3(projectile->target) - this_lt->position);
                quick_emitter(payload->scene, "Warlock Basic Hit", v3(projectile->target) + v3(0.5f), hit_emitter, 0.1f);
                for (auto& enemy : payload->scene->get_enemies(projectile->target)) {
                    auto health = payload->scene->registry.try_get<Health>(enemy);
                    if (!health)
                        continue;
                    auto enemy_lt = payload->scene->registry.try_get<LogicTransform>(enemy);
                    v3 damage_dir = v3(0.0f);
                    if (this_lt && enemy_lt)
                        damage_dir = enemy_lt->position - this_lt->position;
                    health->damage(2.0f, damage_dir);
                }
            }
        },
        .payload = new WarlockProjectilePayload{ability.scene, ability.caster},
        .payload_owned = true
    };
    quick_projectile(ability.scene, projectile, pot_pos, "emitters/warlock/basic_proj.sbemt");
        

    auto poser = ability.scene->registry.try_get<PoseController>(ability.caster);
    if (poser) {
        poser->set_state(AnimationState_AttackOut, ability.post_trigger_time.value());
    }
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

void warlock_attack_end(void* payload) {
    auto ability = id_ptr<Ability>((u64) payload);
    auto poser = ability->scene->registry.try_get<PoseController>(ability->caster);
    if (poser) {
        poser->set_state(AnimationState_Idle);
    }
}


void build_warlock(Scene* scene, entt::entity entity, const LizardPrefab& lizard_prefab) {
    auto& liz = scene->registry.emplace<Lizard>(entity, lizard_prefab.type, lizard_prefab.default_direction);
    
    liz.basic_ability = make_ability(scene, "Warlock Basic");
    liz.basic_ability->caster = entity;
    liz.basic_ability->pre_trigger_time = Stat(0.7f);
    liz.basic_ability->post_trigger_time = Stat(1.0f);
    liz.basic_ability->cooldown_time = Stat(1.2f);
    liz.basic_ability->start_callback = warlock_attack_start;
    liz.basic_ability->start_payload = (void*) liz.basic_ability.id;
    liz.basic_ability->trigger_callback = warlock_attack_trigger;
    liz.basic_ability->trigger_payload = (void*) liz.basic_ability.id;
    liz.basic_ability->end_callback = warlock_attack_end;
    liz.basic_ability->end_payload = (void*) liz.basic_ability.id;
    liz.basic_ability->targeting_callback = warlock_attack_targeting;
    liz.basic_ability->targeting_payload = (void*) liz.basic_ability.id;
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
