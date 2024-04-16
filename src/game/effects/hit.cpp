#include "hit.hpp"

#include "game/scene.hpp"
#include "game/entities/components.hpp"

namespace spellbook {

void Hit::pre_apply() {

}

void Hit::process() {
    Team& caster_team_info = scene->registry.get<Team>(caster);
    bool splash2 = false;
    bool splash3 = false;
    for (HitEffect& effect : effects) {
        if (effect.type == EffectType_Splash) {
            if (effect.level == 1)
                splash2 = true;
            if (effect.level == 2)
                splash3 = true;
        }
    }

    vector<entt::entity> friendly_entities;
    vector<entt::entity> hostile_entities;
    for (auto [entity, team_info, logic_tfm] : scene->registry.view<Team, LogicTransform>().each()) {
        // This would be nice if it was effect-local, but it's a one-off case
        if (splash3) {
            if (math::round_cast(logic_tfm.position.z) != position.z)
                continue;
        }
        else if (splash2) {
            if (math::distance(math::round(logic_tfm.position), v3(position)) <= 1.0f)
                continue;
        }
        else {
            if (math::round_cast(logic_tfm.position) != position)
                continue;
        }
        if (caster_team_info.outlook[team_info.identity] == Team::Disposition_Friendly) {
            friendly_entities.push_back(entity);
        }
        if (caster_team_info.outlook[team_info.identity] == Team::Disposition_Hostile) {
            hostile_entities.push_back(entity);
        }
    }

    pre_apply();

    for (HitEffect& effect : effects) {
        if (!effect.hits_disposition_function)
            continue;
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