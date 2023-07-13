#pragma once

#include <entt/entity/fwd.hpp>

#include "general/string.hpp"
#include "general/json.hpp"
#include "general/math/geometry.hpp"

namespace spellbook {

enum TileType {
    TileType_Empty,
    TileType_TowerSlot,
    TileType_Path,
    TileType_Scenery,
    TileType_Ramp,
    TileType_CastingPlatform
};

struct TilePrefab {
    string file_path;
    vector<string> dependencies;
    
    TileType type;
    string model_path;
    v3 visual_offset = v3(0.5f, 0.5f, 0.0f);
    uset<v3i> solids;

    v3i new_offset;
};

JSON_IMPL(TilePrefab, type, model_path, visual_offset, solids);

struct Scene;

bool inspect(TilePrefab*);
entt::entity instance_prefab(Scene* scene, const TilePrefab& tile_prefab, v3i location, uint32 rotation = 0);

}