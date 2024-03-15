#pragma once

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
    umap<v3i, TileEntry> tiles;
    umap<v3i, FilePath> spawners;
    umap<v3i, FilePath> consumers;
    umap<v3i, uint8> solid_tiles;

    static constexpr string_view extension() { return ".sbjmap"; }
    static constexpr string_view dnd_key() { return "DND_MAP"; }
    static FilePath folder() { return "maps"_resource; }
    static std::function<bool(const FilePath&)> path_filter() { return [](const FilePath& path) { return path.extension() == MapPrefab::extension(); }; }
};
JSON_IMPL(MapPrefab::TileEntry, prefab_path, rotation);
JSON_IMPL(MapPrefab, tiles, spawners, consumers, solid_tiles);

bool inspect(MapPrefab* prefab);
Scene* instance_map(const MapPrefab&, const string& name);


}
