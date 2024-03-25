#include "hit.hpp"

#include "game/scene.hpp"
#include "game/entities/components.hpp"

namespace spellbook {

void Hit::pre_apply() {

}

void Hit::process() {
    Team& caster_team_info = scene->registry.get<Team>(caster);
    vector<entt::entity> friendly_entities;
    vector<entt::entity> hostile_entities;
    for (auto [entity, team_info, logic_tfm] : scene->registry.view<Team, LogicTransform>().each()) {
        if (math::round_cast(logic_tfm.position) != position)
            continue;
        if (caster_team_info.outlook[team_info.identity] == Team::Disposition_Friendly) {
            friendly_entities.push_back(entity);
        }
        if (caster_team_info.outlook[team_info.identity] == Team::Disposition_Hostile) {
            hostile_entities.push_back(entity);
        }
    }

    pre_apply();

    for (HitEffect& effect : effects) {
        if (effect.hits_disposition_function(Team::Disposition_Friendly))
            effect.apply_function(effect, scene, caster, friendly_entities);
        if (effect.hits_disposition_function(Team::Disposition_Hostile))
            effect.apply_function(effect, scene, caster, hostile_entities);
    }

    post_apply();
}

void Hit::post_apply() {

}

}