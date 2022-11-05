#include "map.hpp"

#include <entt/entt.hpp>

#include "components.hpp"
#include "console.hpp"
#include "file.hpp"
#include "imgui.h"

namespace fs = std::filesystem;

namespace spellbook {


void inspect(MapPrefab* map_prefab) {
    ImGui::Text("Towers");
    u32 tower_i = 0;
    for (auto& [pos, prefab] : map_prefab->towers) {
        ImGui::Text("%d", tower_i++);
        ImGui::Indent();
        inspect(&prefab);
        ImGui::Unindent();
    }
    ImGui::Separator();

    ImGui::Text("Tiles");
    u32 tile_i = 0;
    for (auto& [pos, prefab] : map_prefab->tiles) {
        ImGui::Text("%d", tile_i++);
        ImGui::Indent();
        inspect(&prefab);
        ImGui::Unindent();
    }
    ImGui::Separator();

    ImGui::Text("Spawners");
    u32 spawner_i = 0;
    for (auto& [pos, prefab] : map_prefab->spawners) {
        ImGui::Text("%d", spawner_i++);
        ImGui::Indent();
        inspect(&prefab);
        ImGui::Unindent();
    }
    ImGui::Separator();

    ImGui::Text("Consumers");
    u32 consumer_i = 0;
    for (auto& [pos, prefab] : map_prefab->consumers) {
        ImGui::Text("%d", consumer_i++);
        ImGui::Indent();
        inspect(&prefab);
        ImGui::Unindent();
    }
    ImGui::Separator();
}

void save_map(const MapPrefab& map_prefab) {
    auto j = from_jv<json>(to_jv(map_prefab));
    
    string ext = fs::path(map_prefab.file_path).extension().string();
    assert_else(ext == extension(FileType_Map));
    
    file_dump(j, to_resource_path(map_prefab.file_path).string());
}

MapPrefab load_map(const string& input_path) {
    fs::path absolute_path = to_resource_path(input_path);
    warn_else(fs::exists(absolute_path))
        return {};
    string ext = absolute_path.extension().string();
    assert_else(ext == extension(FileType_Map))
        return {};

    json j = parse_file(absolute_path.string());
    auto map_prefab = from_jv<MapPrefab>(to_jv(j));
    map_prefab.file_path = absolute_path.string();
    return map_prefab;
}

Scene* instance_map(const MapPrefab& map_prefab) {
    auto scene = new Scene();
    scene->setup();
    for (auto& [pos, prefab] : map_prefab.towers) {
        instance_prefab(scene, prefab, pos);
    }
    for (auto& [pos, prefab] : map_prefab.tiles) {
        instance_prefab(scene, prefab, pos);
    }
    for (auto& [pos, prefab] : map_prefab.spawners) {
        instance_prefab(scene, prefab, pos);
    }
    for (auto& [pos, prefab] : map_prefab.consumers) {
        instance_prefab(scene, prefab, pos);
    }
    return scene;
}


}
