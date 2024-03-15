#include "hit.hpp"

#include "game/scene.hpp"
#include "game/entities/components.hpp"

namespace spellbook {

void Hit::process() {
    Team& caster_team_info = scene->registry.get<Team>(caster);
    vector<entt::entity> friendly_entities;
    vector<entt::entity> hostile_entities;
    for (auto [entity, team_info, position] : scene->registry.view<Team, LogicTransform>().each()) {
        if (caster_team_info.outlook[team_info.identity] == Team::Disposition_Friendly) {
            friendly_entities.push_back(entity);
        }
        if (caster_team_info.outlook[team_info.identity] == Team::Disposition_Hostile) {
            hostile_entities.push_back(entity);
        }
    }


}

}