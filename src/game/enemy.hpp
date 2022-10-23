#pragma once

#include "string.hpp"
#include "json.hpp"

namespace spellbook {

enum EnemyType {
    EnemyType_Empty,
    EnemyType_Banana
};

struct EnemyPrefab {
    EnemyType type = EnemyType_Empty;
    string model_path = "";
    string file_path = "";

    f32 max_health = 0.0f;
};

JSON_IMPL(EnemyPrefab, type, model_path);

}