#pragma once

#include <entt/fwd.hpp>

#include "general/string.hpp"
#include "general/json.hpp"
#include "general/geometry.hpp"

namespace spellbook {

enum TileType {
    TileType_Empty,
    TileType_TowerSlot,
    TileType_Path,
    TileType_Scenery
};

struct TilePrefab {
    TileType type;
    string model_path;
    string file_path;
};

JSON_IMPL(TilePrefab, type, model_path);

struct Scene;
entt::entity instance_prefab(Scene*, const TilePrefab&, v3i location);

void inspect(TilePrefab*);
void save_tile(const TilePrefab&);
TilePrefab load_tile(const string& input_path);
entt::entity instance_prefab(Scene* scene, const TilePrefab& tile_prefab, v3i location);

}