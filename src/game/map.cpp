﻿#include "map.hpp"

#include <entt/entt.hpp>
#include <imgui.h>

#include "general/logger.hpp"
#include "game/components.hpp"
#include "game/game_file.hpp"
#include "extension/imgui_extra.hpp"

namespace fs = std::filesystem;

namespace spellbook {

void inspect(MapPrefab* map_prefab) {
    ImGui::Text("Lizards");
    u32 lizard_i = 0;
    for (auto& [pos, prefab] : map_prefab->lizards) {
        ImGui::Text("%d", lizard_i++);
        ImGui::Indent();
        ImGui::PushID(prefab.c_str());
        ImGui::PathSelect("Path", &prefab, "resources/lizards", FileType_Lizard);
        ImGui::PopID();
        ImGui::Unindent();
    }
    ImGui::Separator();

    ImGui::Text("Tiles");
    u32 tile_i = 0;
    for (auto& [pos, entry] : map_prefab->tiles) {
        ImGui::Text("%d", tile_i++);
        ImGui::Indent();
        ImGui::PushID(entry.prefab_path.c_str());
        ImGui::PathSelect("Path", &entry.prefab_path, "resources/tiles", FileType_Tile);
        int rot = entry.rotation;
        if (ImGui::SliderInt("Rotation", &rot, 0, 3))
            entry.rotation = rot;
        ImGui::PopID();
        ImGui::Unindent();
    }
    ImGui::Separator();

    ImGui::Text("Spawners");
    u32 spawner_i = 0;
    for (auto& [pos, prefab] : map_prefab->spawners) {
        ImGui::Text("%d", spawner_i++);
        ImGui::Indent();
        ImGui::PushID(prefab.c_str());
        ImGui::PathSelect("Path", &prefab, "resources/spawners", FileType_Spawner);
        ImGui::PopID();
        ImGui::Unindent();
    }
    ImGui::Separator();

    ImGui::Text("Consumers");
    u32 consumer_i = 0;
    for (auto& [pos, prefab] : map_prefab->consumers) {
        ImGui::Text("%d", consumer_i++);
        ImGui::Indent();
        ImGui::PushID(prefab.c_str());
        ImGui::PathSelect("Path", &prefab, "resources/consumers", FileType_Consumer);
        ImGui::PopID();
        ImGui::Unindent();
    }
    ImGui::Separator();
}

Scene* instance_map(const MapPrefab& map_prefab, const string& name) {
    auto scene = new Scene();
    scene->setup(name);
    for (auto& [pos, prefab] : map_prefab.lizards) {
        if (prefab.empty())
            continue;
        instance_prefab(scene, load_asset<LizardPrefab>(prefab), pos);
    }
    for (auto& [pos, entry] : map_prefab.tiles) {
        if (entry.prefab_path.empty())
            continue;
        instance_prefab(scene, load_asset<TilePrefab>(entry.prefab_path), pos, entry.rotation);
    }
    for (auto& [pos, prefab] : map_prefab.spawners) {
        if (prefab.empty())
            continue;
        instance_prefab(scene, load_asset<SpawnerPrefab>(prefab), pos);
    }
    for (auto& [pos, prefab] : map_prefab.consumers) {
        if (prefab.empty())
            continue;
        instance_prefab(scene, load_asset<ConsumerPrefab>(prefab), pos);
    }
    return scene;
}


}
