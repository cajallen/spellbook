#include "hit.hpp"

#include "game/scene.hpp"
#include "game/entities/components.hpp"

namespace spellbook {

void Hit::do_reactions() {
    for (HitReaction& reaction : scene->hit_scene_state.reactions) {
        if (scene->registry.valid(reaction.relies_on_propagator))
            if (reaction.relies_on_propagator != caster)
                continue;

        for (HitEffect& effect : effects) {
            if (reaction.relies_on_effect_type != effect.type)
                continue;

            for (entt::entity entity :  reaction.requires_special_activation ? effect.affected_entities : effect.checked_entities) {
                if (scene->registry.valid(reaction.relies_on_receiver))
                    if (reaction.relies_on_receiver != entity)
                        continue;

                reaction.reaction(scene, caster, entity, effect);
            }
        }
    }
}

void Hit::pre_apply() {

}

void Hit::process() {
    Team& caster_team_info = scene->registry.get<Team>(caster);
    bool splash2 = false;
    bool floor = false;
    for (HitEffect& effect : effects) {
        if (effect.type == EffectType_Splash)
            if (effect.level == 1)
                splash2 = true;
        if (effect.type == EffectType_Floor)
            if (effect.level == 2)
                floor = true;
    }

    for (auto [entity, team_info, logic_tfm] : scene->registry.view<Team, LogicTransform>().each()) {
        // This would be nice if it was effect-local, but it's a one-off case
        if (floor) {
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
        for (Team::Disposition disposition : {Team::Disposition_Friendly, Team::Disposition_Hostile}) {
            if (caster_team_info.outlook[team_info.identity] == disposition) {
                hit_entities[disposition].push_back(entity);
            }
        }
    }

    pre_apply();

    for (HitEffect& effect : effects) {
        if (!effect.hits_disposition_function)
            continue;
        for (auto [disposition, entities] : hit_entities) {
            if (effect.hits_disposition_function(disposition))
                effect.apply_function(effect, scene, caster, entities);
        }
    }

    post_apply();
}

void Hit::post_apply() {
    do_reactions();
}

void HitSceneState::cleanup_on_death(entt::entity entity) {
    for (auto it = reactions.begin(); it != reactions.end();) {
        if (it->relies_on_propagator == entity || it->relies_on_receiver == entity)
            it = reactions.remove(it);
        else
            it++;
    }
}

}