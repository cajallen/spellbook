#pragma once

#include <entt/entity/entity.hpp>

#include "game/entities/stat.hpp"

namespace spellbook {

struct Scene;

struct Projectile {
    v3i target;
    Stat speed = Stat(1.0f);
    v3 alignment = v3(0.0f);

    void(*callback)(entt::entity proj_entity, void*) = {};
    void* payload = nullptr;
    bool payload_owned = false;
};

entt::entity quick_projectile(Scene* scene, Projectile proj, v3 pos, const string& particles_path = "", const string& model_path = "", float scale = 1.0f);

void projectile_system(Scene* scene);

}
