#include "map.hpp"

#include <entt/entt.hpp>
#include <imgui.h>

#include "general/logger.hpp"
#include "game/components.hpp"
#include "game/game_file.hpp"

namespace fs = std::filesystem;

namespace spellbook {

void inspect(MapPrefab* map_prefab) {
    ImGui::Text("Lizards");
    u32 lizard_i = 0;
    for (auto& [pos, prefab] : map_prefab->lizards) {
        ImGui::Text("%d", lizard_i++);
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

Scene* instance_map(const MapPrefab& map_prefab, const string& name) {
    auto scene = new Scene();
    scene->setup(name);
    for (auto& [pos, prefab] : map_prefab.lizards) {
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
