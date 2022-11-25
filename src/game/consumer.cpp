#include "consumer.hpp"

#include <entt/entt.hpp>
#include <imgui.h>
#include "extension/imgui_extra.hpp"
#include "editor/asset_browser.hpp"
#include "game/scene.hpp"
#include "game/components.hpp"

namespace spellbook {

entt::entity instance_prefab(Scene* scene, const ConsumerPrefab& consumer_prefab, v3i location) {
    static int i      = 0;
    
    auto       entity = scene->registry.create();
    scene->registry.emplace<Name>(entity, fmt_("{}_{}", "consumer", i++));

    auto& model_comp = scene->registry.emplace<Model>(entity);
    model_comp.model_cpu = load_model(consumer_prefab.model_path);
    model_comp.model_gpu = instance_model(scene->render_scene, model_comp.model_cpu);
    
    scene->registry.emplace<LogicTransform>(entity, v3(location));
    scene->registry.emplace<ModelTransform>(entity);
    scene->registry.emplace<TransformLink>(entity, v3(0.5f));
    scene->registry.emplace<Consumer>(entity);

    return entity;
}

void inspect(ConsumerPrefab* consumer_prefab) {
    ImGui::PathSelect("File", &consumer_prefab->file_path, "resources", FileType_Consumer, true);
    ImGui::PathSelect("Model", &consumer_prefab->model_path, "resources", FileType_Model, true);
}

void save_consumer(const ConsumerPrefab& consumer_prefab) {
    auto j = from_jv<json>(to_jv(consumer_prefab));
    
    string ext = fs::path(consumer_prefab.file_path).extension().string();
    assert_else(ext == extension(FileType_Consumer));
    
    file_dump(j, to_resource_path(consumer_prefab.file_path).string());
}

ConsumerPrefab load_consumer(const string& input_path) {
    fs::path absolute_path = to_resource_path(input_path);
    warn_else(fs::exists(absolute_path))
        return {};
    string ext = absolute_path.extension().string();
    assert_else(ext == extension(FileType_Consumer))
        return {};

    json j = parse_file(absolute_path.string());
    auto consumer_prefab = from_jv<ConsumerPrefab>(to_jv(j));
    consumer_prefab.file_path = absolute_path.string();
    return consumer_prefab;
}

}
