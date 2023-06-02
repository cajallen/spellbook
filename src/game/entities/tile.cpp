#include "tile.hpp"

#include <entt/entity/entity.hpp>
#include <imgui.h>

#include "extension/fmt.hpp"
#include "extension/imgui_extra.hpp"
#include "general/logger.hpp"
#include "game/scene.hpp"
#include "game/entities/components.hpp"

namespace spellbook {

entt::entity instance_prefab(Scene* scene, const TilePrefab& tile_prefab, v3i location, u32 rotation) {
    static int i      = 0;
    
    auto       entity = scene->registry.create();

    if (!tile_prefab.model_path.empty()) {
        auto& model_comp = scene->registry.emplace<Model>(entity);
        model_comp.model_cpu = std::make_unique<ModelCPU>(load_asset<ModelCPU>(tile_prefab.model_path));
        model_comp.model_gpu = instance_model(scene->render_scene, *model_comp.model_cpu);
        scene->registry.emplace<ModelTransform>(entity, v3(location), quat(v3::Z, math::PI * 0.5f * float(rotation)));
        scene->registry.emplace<TransformLink>(entity, tile_prefab.visual_offset);
    }
    scene->registry.emplace<LogicTransform>(entity, v3(location), euler{.yaw = math::PI * 0.5f * float(rotation)});
    switch (tile_prefab.type) {
        case (TileType_TowerSlot): {
            scene->registry.emplace<GridSlot>(entity, false, false, true);
        } break;
        case (TileType_Path): {
            scene->registry.emplace<GridSlot>(entity, true, false, true);
        } break;
        case (TileType_Ramp): {
            scene->registry.emplace<Name>(entity, fmt_("{}_{}", "tile", i++));
            switch (rotation) {
                case 0: {
                    scene->registry.emplace<GridSlot>(entity, true, true, true, Direction_PosX);
                } break;
                case 1: {
                    scene->registry.emplace<GridSlot>(entity, true, true, true, Direction_PosY);
                } break;
                case 2: {
                    scene->registry.emplace<GridSlot>(entity, true, true, true, Direction_NegX);
                } break;
                case 3: {
                    scene->registry.emplace<GridSlot>(entity, true, true, true, Direction_NegY);
                } break;
            }
        } break;
        case (TileType_Scenery): {
            scene->registry.emplace<GridSlot>(entity, false, false, false);
        } break;
        case (TileType_CastingPlatform): {
            scene->registry.emplace<Name>(entity, fmt_("{}_{}", "tile", i++));
            scene->registry.emplace<CastingPlatform>(entity);
            return entity;
        }
    }
    umap<v3i, entt::entity> entity_map;
    entity_map[v3i(0)] = entity;

    quat r = quat(v3::Z, math::PI * 0.5f * float(rotation));
    for (const v3i& offset : tile_prefab.solids) {
        if (offset == v3i(0))
            continue;
        auto extra_entity = scene->registry.create();
        scene->registry.emplace<LogicTransform>(extra_entity, v3(location) + math::rotate(r, v3(offset)));
        switch (tile_prefab.type) {
            case (TileType_Scenery): {
                scene->registry.emplace<GridSlot>(extra_entity, false, false, false);
            } break;
            default: {
                log_error("NYI");
            }
        }
        entity_map[offset] = extra_entity;
    }

    for (auto& [offset, e] : entity_map) {
        uset<entt::entity>& linked = scene->registry.get<GridSlot>(e).linked;
        for (auto& [offset2, e2] : entity_map) {
            if (e == e2)
                continue;
            linked.insert(e2);
        }
    }

    return entity;
}

bool inspect(TilePrefab* tile_prefab) {
    bool changed = false;
    ImGui::PathSelect("File", &tile_prefab->file_path, "resources/tiles", FileType_Tile, true);
    changed |= ImGui::EnumCombo("Type", &tile_prefab->type);
    changed |= ImGui::PathSelect("Model", &tile_prefab->model_path, "resources/models", FileType_Model, true);
    changed |= ImGui::DragFloat3("Visual Offset", tile_prefab->visual_offset.data, 0.01f);

    ImGui::Text("Extra Solids");
    ImGui::Indent();
    if (tile_prefab->solids.empty())
        ImGui::Text("None");
    for (auto it = tile_prefab->solids.begin(); it != tile_prefab->solids.end();) {
        ImGui::Text("%d, %d, %d", it->x, it->y, it->z);
        ImGui::SameLine();
        ImGui::Dummy({ImGui::GetContentRegionAvail().x - 40.0f, 0.0f});
        ImGui::SameLine();
        ImGui::PushID(&*it);
        if (ImGui::Button(ICON_FA_TIMES, {24.f, 0.f})) {
            it = tile_prefab->solids.erase(it);
            changed = true;
        } else {
            it++;
        }
        ImGui::PopID();
    }
    ImGui::Unindent();
    
    ImGui::DragInt3("Offset", tile_prefab->new_offset.data, 0.05f);
    ImGui::SameLine();
    ImGui::Dummy({ImGui::GetContentRegionAvail().x - 50.0f, 0.0f});
    ImGui::SameLine();
    if (ImGui::Button("Add")) {
        tile_prefab->solids.insert(tile_prefab->new_offset);
        changed = true;
    }
    
    return changed;
}

}
