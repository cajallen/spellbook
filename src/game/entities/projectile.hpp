﻿#pragma once

#include <entt/entity/entity.hpp>

#include "game/entities/stat.hpp"

namespace spellbook {

struct Scene;

struct Projectile {
    v3i target;
    StatInstance speed;
    v3 alignment = v3(0.0f);

    bool first_frame = true;
    
    std::function<void(entt::entity proj_entity)> callback = {};
};

entt::entity quick_projectile(Scene* scene, Projectile proj, v3 pos, const string& particles_path = "", const string& model_path = "", float scale = 1.0f);

void projectile_system(Scene* scene);

}