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
    string globe_path;
    string clouds_path;
    string file_path;
};

JSON_IMPL(TowerPrefab, type, globe_path, clouds_path);

struct Scene;
entt::entity instance_prefab(Scene*, const TowerPrefab&, v3i location);
void inspect(TowerPrefab*);
void save_tower(const TowerPrefab&);
TowerPrefab load_tower(const string& input_path);

}