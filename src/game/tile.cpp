#include "tile.hpp"

#include <entt/entt.hpp>
#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "asset_browser.hpp"
#include "lib_ext/imgui_extra.hpp"

#include "scene.hpp"
#include "components.hpp"
#include "input.hpp"

namespace spellbook {

entt::entity instance_prefab(Scene* scene, const TilePrefab& tile_prefab, v3i location) {
    static int i      = 0;
    
    auto       entity = scene->registry.create();
    scene->registry.emplace<Name>(entity, fmt_("{}_{}", "brush", i++));

    auto& model_comp = scene->registry.emplace<Model>(entity);
    model_comp.model_cpu = load_model(tile_prefab.model_path);
    model_comp.model_gpu = instance_model(scene->render_scene, model_comp.model_cpu);
    
    scene->registry.emplace<LogicTransform>(entity, v3(location));
    scene->registry.emplace<ModelTransform>(entity, v3(location));
    scene->registry.emplace<GridSlot>(entity, tile_prefab.type == TileType_Path);

    return entity;
}

void inspect(TilePrefab* tile_prefab) {
    PathSelect("File", &tile_prefab->file_path, "resources", FileType_Tile, true);
    EnumCombo("Type", &tile_prefab->type);
    PathSelect("Model", &tile_prefab->model_path, "resources", FileType_Model, true);
}

}
