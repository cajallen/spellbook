#include "asset_editor.hpp"

#include <tracy/Tracy.hpp>

#include "game.hpp"
#include "imgui/misc/cpp/imgui_stdlib.h"

#include "lib_ext/imgui_extra.hpp"

#include "input.hpp"

#include "game/components.hpp"

namespace spellbook {

void AssetEditor::setup() {
    ZoneScoped;
    p_scene = new Scene();

    entity = p_scene->registry.create();
    p_scene->registry.emplace<Name>(entity, "Asset");
    p_scene->registry.emplace<Model>(entity);
    p_scene->registry.emplace<Transform>(entity);
}

void AssetEditor::update() {
    ZoneScoped;
    if (Input::mouse_click[GLFW_MOUSE_BUTTON_LEFT]) {
        p_scene->render_scene.query = v2i(Input::mouse_pos) - p_scene->render_scene.viewport.start;
    }
}

void AssetEditor::window(bool* p_open) {
    ZoneScoped;
    auto& model_comp = p_scene->registry.get<Model>(entity);
    if (ImGui::Begin("Asset Editor", p_open)) {
        PathSelect("File##Convert", &convert_file, "external_resources", "DND_PREFAB");
        if (ImGui::Button("Convert")) {
            save_model(convert_to_model(convert_file.string(), "models", "model"));
        }
        ImGui::Separator();
        PathSelect("File##Load", &load_file, "resources", "DND_PREFAB");
        if (ImGui::Button("Load")) {
            if (model_comp.model_gpu.renderables.size() > 0) {
                deinstance_model(p_scene->render_scene, model_comp.model_gpu);
            }
            model_comp.model_cpu = load_model(load_file.string());
            model_comp.model_gpu = instance_model(p_scene->render_scene, model_comp.model_cpu);
        }
        ImGui::Separator();
        inspect(&model_comp.model_cpu);
    }
    ImGui::End();

    static auto render_scene = new RenderScene();
    static bool initialized = false;
    
    if (initialized)
        render_scene->pause = true;
    
    if (!initialized && !model_comp.model_cpu.file_name.empty()) {
        render_scene->name = "model_thumbnail";
        render_scene->viewport.name	 = render_scene->name + "::viewport";
        render_scene->viewport.camera = new Camera(v3(-8, 0, 4), math::d2r(euler{0, -30}));
        render_scene->viewport.setup();

        ModelGPU model_gpu = instance_model(*render_scene, model_comp.model_cpu);
        
        game.renderer.add_scene(render_scene);
        initialized = true;
    }
}


}