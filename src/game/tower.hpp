#pragma once

#include <filesystem>

namespace fs = std::filesystem;

namespace spellbook {

enum TowerType {
    TowerType_Empty,
    TowerType_Roller,
    TowerType_Pyro,
    TowerType_Count
};

struct TowerPrefab {
    TowerType type;
    fs::path model_path;
    fs::path file_path;
};

JSON_IMPL(TowerPrefab, type, model_path);

}