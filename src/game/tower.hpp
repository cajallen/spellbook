#pragma once

#include <entt/fwd.hpp>

#include "string.hpp"
#include "json.hpp"
#include "geometry.hpp"

namespace spellbook {

enum TowerType {
    TowerType_Empty,
    TowerType_Roller,
    TowerType_Pyro,
    TowerType_Count
};

struct TowerPrefab {
    TowerType type;
    string model_path;
    string file_path;
};

JSON_IMPL(TowerPrefab, type, model_path);

struct Scene;
entt::entity instance_prefab(Scene*, const TowerPrefab&, v3i location);

void inspect(TowerPrefab*);

}