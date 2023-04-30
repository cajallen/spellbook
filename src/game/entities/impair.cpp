#include "impair.hpp"

#include "game/scene.hpp"

namespace spellbook {

bool Impairs::is_impaired(Scene* scene, ImpairType type) {
    for (auto& [_, active_type] : untimed_impairs)
        if (type == active_type)
            return true;

    for (auto it = timed_impairs.begin(); it != timed_impairs.end();) {
        if (it->second.until < scene->time) {
            it = timed_impairs.erase(it);
            continue;
        }

        if (type == it->second.type)
            return true;
    }
    return false;
}

void apply_untimed_impair(Impairs& impairs, u64 id, ImpairType type) {
    impairs.untimed_impairs[id] = type;
}

void apply_timed_impair(Scene* scene, entt::entity entity, u64 id, ImpairType type, float time) {
    Impairs& impairs = scene->registry.get<Impairs>(entity);
    if (impairs.timed_impairs.contains(id)) {
        if (impairs.timed_impairs[id].until > scene->time + time)
            return;
    }
    impairs.timed_impairs[id] = {type, scene->time + time};
}

}