#pragma once

#include "umap.hpp"
#include "geometry.hpp"

#include "game/tower.hpp"
#include "game/tile.hpp"
#include "game/enemy.hpp"

namespace spellbook {

struct Map {
    string file_path;
    
    umap<v3i, TowerPrefab> towers;
    umap<v3i, TilePrefab> tiles;
};

JSON_IMPL(Map, towers, tiles);

}