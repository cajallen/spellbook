#include "map.hpp"

#include <imgui/imgui.h>

#include "extension/imgui_extra.hpp"
#include "general/logger.hpp"
#include "game/game_file.hpp"
#include "game/scene.hpp"
#include "game/entities/components.hpp"
#include "game/entities/tile.hpp"
#include "game/entities/spawner.hpp"
#include "game/entities/consumer.hpp"

namespace fs = std::filesystem;

namespace spellbook {

bool inspect(MapPrefab* map_prefab) {
    ImGui::PathSelect("File", &map_prefab->file_path, FileType_Map);
    inspect_dependencies(map_prefab->dependencies, map_prefab->file_path);
    
    ImGui::Text("Lizards");
    uint32 lizard_i = 0;
    for (auto& [pos, prefab] : map_prefab->lizards) {
        ImGui::Text("%d", lizard_i++);
        ImGui::Indent();
        ImGui::PushID(prefab.c_str());
        ImGui::PathSelect("Path", &prefab, FileType_Lizard);
        ImGui::PopID();
        ImGui::Unindent();
    }
    ImGui::Separator();

    if (ImGui::TreeNode("Tiles")) {
        uint32 tile_i = 0;
        for (auto& [pos, entry] : map_prefab->tiles) {
            ImGui::Text("%d", tile_i++);
            ImGui::Indent();
            ImGui::PushID(entry.prefab_path.c_str());
            ImGui::PathSelect("Path", &entry.prefab_path, FileType_Tile);
            int rot = entry.rotation;
            if (ImGui::SliderInt("Rotation", &rot, 0, 3))
                entry.rotation = rot;
            ImGui::PopID();
            ImGui::Unindent();
        }
        ImGui::TreePop();
    }
    ImGui::Separator();

    ImGui::Text("Spawners");
    uint32 spawner_i = 0;
    for (auto& [pos, prefab] : map_prefab->spawners) {
        ImGui::Text("%d", spawner_i++);
        ImGui::Indent();
        ImGui::PushID(prefab.c_str());
        ImGui::PathSelect("Path", &prefab, FileType_Spawner);
        ImGui::PopID();
        ImGui::Unindent();
    }
    ImGui::Separator();

    ImGui::Text("Consumers");
    uint32 consumer_i = 0;
    for (auto& [pos, prefab] : map_prefab->consumers) {
        ImGui::Text("%d", consumer_i++);
        ImGui::Indent();
        ImGui::PushID(prefab.c_str());
        ImGui::PathSelect("Path", &prefab, FileType_Consumer);
        ImGui::PopID();
        ImGui::Unindent();
    }
    ImGui::Separator();

    return false;
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
