#pragma once

#include <entt/entity/fwd.hpp>
#include "general/umap.hpp"
#include "general/math/geometry.hpp"
#include "general/file/resource.hpp"

namespace spellbook {

struct Scene;

struct MapPrefab : Resource {
    struct TileEntry {
        FilePath prefab_path;
        uint32 rotation;
    };
    struct HomeEntry {
        v3i position;
        uint32 rotation;
        entt::entity e;
    };
    umap<v3i, TileEntry> tiles;
    umap<v3i, FilePath> spawners;
    umap<v3i, FilePath> consumers;
    umap<v3i, uint8> solid_tiles;
    vector<HomeEntry> homes;

    static constexpr string_view extension() { return ".sbjmap"; }
    static constexpr string_view dnd_key() { return "DND_MAP"; }
    static FilePath folder() { return "maps"_resource; }
    static function<bool(const FilePath&)> path_filter() { return [](const FilePath& path) { return path.extension() == MapPrefab::extension(); }; }
};
JSON_IMPL(MapPrefab::TileEntry, prefab_path, rotation);
JSON_IMPL(MapPrefab::HomeEntry, position, rotation);
JSON_IMPL(MapPrefab, tiles, spawners, consumers, solid_tiles, homes);

bool inspect(MapPrefab* prefab);
Scene* instance_map(const MapPrefab&, const string& name);


}
