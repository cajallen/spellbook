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

inline json_value to_jv(const fs::path& input_path) {
    json_value jv;
    jv.value = json_variant {input_path.string()};
    return jv;
}
inline fs::path from_jv_impl(const json_value& jv, fs::path* _) {
    return fs::path(get<string>(jv.value));
}
JSON_IMPL(TowerPrefab, type, model_path);

}