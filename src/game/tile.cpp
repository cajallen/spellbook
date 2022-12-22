#include "tile.hpp"

#include <entt/entt.hpp>
#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "extension/fmt.hpp"
#include "extension/imgui_extra.hpp"
#include "general/logger.hpp"
#include "game/scene.hpp"
#include "game/components.hpp"

namespace spellbook {

entt::entity instance_prefab(Scene* scene, const TilePrefab& tile_prefab, v3i location, u32 rotation) {
    static int i      = 0;
    
    auto       entity = scene->registry.create();
    scene->registry.emplace<Name>(entity, fmt_("{}_{}", "tile", i++));

    auto& model_comp = scene->registry.emplace<Model>(entity);
    model_comp.model_cpu = load_asset<ModelCPU>(tile_prefab.model_path);
    model_comp.model_gpu = std::move(instance_model(scene->render_scene, model_comp.model_cpu));

    scene->registry.emplace<LogicTransform>(entity, v3(location));
    scene->registry.emplace<ModelTransform>(entity, v3(location), euler{float(rotation) * math::PI / 2.0f});
    scene->registry.emplace<TransformLink>(entity, v3(0.5f));
    scene->registry.emplace<GridSlot>(entity, tile_prefab.type == TileType_Path);

    return entity;
}

void inspect(TilePrefab* tile_prefab) {
    ImGui::PathSelect("File", &tile_prefab->file_path, "resources", FileType_Tile, true);
    ImGui::EnumCombo("Type", &tile_prefab->type);
    ImGui::PathSelect("Model", &tile_prefab->model_path, "resources", FileType_Model, true);
}

}
