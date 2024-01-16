#pragma once

#include <entt/entity/fwd.hpp>

#include "general/string.hpp"
#include "general/math/geometry.hpp"
#include "general/file/resource.hpp"

namespace spellbook {

enum TileType {
    TileType_Empty,
    TileType_TowerSlot,
    TileType_Path,
    TileType_Scenery,
    TileType_Ramp,
    TileType_CastingPlatform
};

struct TilePrefab : Resource {
    TileType type;
    FilePath model_path;
    v3 visual_offset = v3(0.5f, 0.5f, 0.0f);
    uset<v3i> solids;

    v3i new_offset;

    static constexpr string_view extension() { return ".sbjtil"; }
    static constexpr string_view dnd_key() { return "DND_TILE"; }
    static FilePath folder() { return "tiles"_resource; }
    static std::function<bool(const FilePath&)> path_filter() { return [](const FilePath& path) { return path.extension() == TilePrefab::extension(); }; }
};

JSON_IMPL(TilePrefab, type, model_path, visual_offset, solids);

struct Scene;

bool inspect(TilePrefab*);
entt::entity instance_prefab(Scene* scene, const TilePrefab& tile_prefab, v3i location, uint32 rotation = 0);

}