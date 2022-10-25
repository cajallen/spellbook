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
    EnumCombo("Type", &tile_prefab->type);
    fs::path model_path = tile_prefab->model_path;
    fs::path file_path = tile_prefab->file_path;
    PathSelect("Model", &model_path, "resources", possible_model, "DND_MODEL", true);
    PathSelect("File", &file_path, "resources", possible_file, "DND_FILE", true);
    tile_prefab->model_path = model_path.string();
    tile_prefab->file_path = file_path.string();
}

}
