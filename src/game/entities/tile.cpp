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
    scene->registry.emplace<Name>(entity, fmt_("{}_{}", "tile", i++));

    auto& model_comp = scene->registry.emplace<Model>(entity);
    model_comp.model_cpu = std::make_unique<ModelCPU>(load_asset<ModelCPU>(tile_prefab.model_path));
    model_comp.model_gpu = instance_model(scene->render_scene, *model_comp.model_cpu);

    scene->registry.emplace<LogicTransform>(entity, v3(location), euler{.yaw = math::PI / 2.0f * rotation});
    scene->registry.emplace<ModelTransform>(entity, v3(location), euler{.yaw = math::PI / 2.0f * rotation});
    auto& link = scene->registry.emplace<TransformLink>(entity, v3(0.5f));
    switch (tile_prefab.type) {
        case (TileType_TowerSlot): {
            scene->registry.emplace<GridSlot>(entity, false, false);
        } break;
        case (TileType_Path): {
            scene->registry.emplace<GridSlot>(entity, true, false);
        } break;
        case (TileType_Ramp): {
            switch (rotation) {
                case 0: {
                    scene->registry.emplace<GridSlot>(entity, true, true, Direction_PosX);
                } break;
                case 1: {
                    scene->registry.emplace<GridSlot>(entity, true, true, Direction_PosY);
                } break;
                case 2: {
                    scene->registry.emplace<GridSlot>(entity, true, true, Direction_NegX);
                } break;
                case 3: {
                    scene->registry.emplace<GridSlot>(entity, true, true, Direction_NegY);
                } break;
            }
        } break;
        case (TileType_Scenery): {
            link.offset.z = 0.0f;
        } break;
    }

    return entity;
}

bool inspect(TilePrefab* tile_prefab) {
    bool changed = false;
    ImGui::PathSelect("File", &tile_prefab->file_path, "resources/tiles", FileType_Tile, true);
    changed |= ImGui::EnumCombo("Type", &tile_prefab->type);
    changed |= ImGui::PathSelect("Model", &tile_prefab->model_path, "resources/models", FileType_Model, true);
    return changed;
}

}
