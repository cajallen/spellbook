#include "tile.hpp"

#include <entt/entt.hpp>
#include <imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include "extension/imgui_extra.hpp"
#include "general/logger.hpp"
#include "game/scene.hpp"
#include "game/components.hpp"

namespace spellbook {

entt::entity instance_prefab(Scene* scene, const TilePrefab& tile_prefab, v3i location) {
    static int i      = 0;
    
    auto       entity = scene->registry.create();
    scene->registry.emplace<Name>(entity, fmt_("{}_{}", "tile", i++));

    auto& model_comp = scene->registry.emplace<Model>(entity);
    model_comp.model_cpu = load_model(tile_prefab.model_path);
    model_comp.model_gpu = std::move(instance_model(scene->render_scene, model_comp.model_cpu));
    
    scene->registry.emplace<LogicTransform>(entity, v3(location));
    scene->registry.emplace<ModelTransform>(entity, v3(location));
    scene->registry.emplace<GridSlot>(entity, tile_prefab.type == TileType_Path);

    return entity;
}

void inspect(TilePrefab* tile_prefab) {
    ImGui::PathSelect("File", &tile_prefab->file_path, "resources", FileType_Tile, true);
    ImGui::EnumCombo("Type", &tile_prefab->type);
    ImGui::PathSelect("Model", &tile_prefab->model_path, "resources", FileType_Model, true);
}

void save_tile(const TilePrefab& tile_prefab) {
    auto j = from_jv<json>(to_jv(tile_prefab));
    
    string ext = fs::path(tile_prefab.file_path).extension().string();
    assert_else(ext == extension(FileType_Tile));
    
    file_dump(j, to_resource_path(tile_prefab.file_path).string());
}

TilePrefab load_tile(const string& input_path) {
    fs::path absolute_path = to_resource_path(input_path);
    check_else(fs::exists(absolute_path))
        return {};
    string ext = absolute_path.extension().string();
    assert_else(ext == extension(FileType_Tile))
        return {};

    json j = parse_file(absolute_path.string());
    auto tile_prefab = from_jv<TilePrefab>(to_jv(j));
    tile_prefab.file_path = absolute_path.string();
    return tile_prefab;
}

}
