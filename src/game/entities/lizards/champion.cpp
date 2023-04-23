#include "game/entities/lizards/lizard_builder.hpp"

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

struct ChampionAttack : Ability {
    void targeting() override;
    void start() override;
    void trigger() override;
};

void ChampionAttack::start() {
    v3i caster_pos = math::round_cast(scene->registry.get<LogicTransform>(caster).position);
    auto lizard = scene->registry.try_get<Lizard>(caster);
    if (lizard) {
        v3 dir_to = math::normalize(v3(target) - v3(caster_pos));
        float ang = math::angle_difference(lizard->default_direction.xy, dir_to.xy);
        scene->registry.get<LogicTransform>(caster).rotation.yaw = ang;
    }
}

void ChampionAttack::trigger() {
    for (auto& enemy : entry_gather_function(*this, target, 0.0f)) {
        auto& enemy_caster = scene->registry.get<Caster>(enemy);
        auto& this_lt = scene->registry.get<LogicTransform>(caster);
        auto& enemy_lt = scene->registry.get<LogicTransform>(enemy);
        damage(scene, caster, enemy, 2.0f, enemy_lt.position - this_lt.position);
        enemy_caster.taunt.set(u64(this), caster);
        
        add_timer(scene, "taunt", [enemy, this](Timer* timer) {
            if (!scene->registry.valid(enemy))
                return;
            auto& enemy_caster = scene->registry.get<Caster>(enemy);
            enemy_caster.taunt.reset(u64(this));
        }, false).start(2.0f);
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

    if (plus_targeting(2, *this, entry_gather_function))
        return;
}

void build_champion(Scene* scene, entt::entity entity, const LizardPrefab& lizard_prefab) {
    scene->registry.emplace<Lizard>(entity, lizard_prefab.type, lizard_prefab.default_direction);
    Caster& caster = scene->registry.get<Caster>(entity);

    caster.attack = std::make_unique<ChampionAttack>();
    caster.attack->setup(scene, entity, 1.3f, 1.1f, Ability::Type_Attack);
    caster.attack->entry_gather_function = square_aoe_entry_gather(1);
}

void draw_champion_dragging_preview(Scene* scene, entt::entity entity) {
    v3 logic_pos = scene->registry.get<Dragging>(entity).potential_logic_position;
    v3i logic_posi = math::round_cast(logic_pos);
    
    vector<FormattedVertex> vertices;
    for (const v2i& offset : {v2i{-2, 0}, v2i{0, -2}, v2i{2, 0}, v2i{0, 2}}) {
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
