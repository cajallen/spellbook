#include "scene.hpp"

#include <tracy/Tracy.hpp>
#include <entt/entt.hpp>

#include "extension/fmt_geometry.hpp"
#include "general/math.hpp"
#include "game/game.hpp"
#include "game/components.hpp"
#include "game/spawner.hpp"
#include "game/systems.hpp"
#include "renderer/draw_functions.hpp"

namespace spellbook {

void Scene::model_cleanup(entt::registry& registry, entt::entity entity) {
    Model& model = registry.get<Model>(entity);
    deinstance_model(render_scene, model.model_gpu);
}

void Scene::dragging_cleanup(entt::registry& registry, entt::entity entity) {
    auto dragging = registry.try_get<Dragging>(entity);
    auto l_transform = registry.try_get<LogicTransform>(entity);
    if (l_transform) {
        l_transform->position = math::round(dragging->logic_position);
    }
}


void Scene::setup(const string& input_name) {
    name = input_name;
	render_scene.name = name + "::render_scene";
	camera = Camera(v3(-8, 0, 4), math::d2r(euler{0, -30}));
	render_scene.viewport.name	 = render_scene.name + "::viewport";
	render_scene.viewport.camera = &camera;
	render_scene.viewport.setup();
	controller.name = name + "::controller";
	controller.setup(&render_scene.viewport, &camera);

    registry.on_destroy<Model>().connect<&Scene::model_cleanup>(*this);
    registry.on_destroy<Dragging>().connect<&Scene::dragging_cleanup>(*this);

    game.scenes.push_back(this);
	game.renderer.add_scene(&render_scene);
}

void Scene::update() {
	ZoneScoped;
    
    delta_time = Input::delta_time * time_scale;
    time += delta_time;
    
	controller.update();

    dragging_update_system(this);
    collision_update_system(this);
    dragging_system(this);
    spawner_system(this);
    travel_system(this);
    lizard_system(this);
    spawner_draw_system(this);
    transform_system(this);
    selection_id_system(this);
    consumer_system(this);
    health_system(this);
    health_draw_system(this);
    disposal_system(this);

    for (auto entity : registry.view<Name>()) {
        preview_3d_components(this, entity);
    }

    render_scene.render_grid = edit_mode;
    render_scene.render_widgets = edit_mode;
}

void Scene::cleanup() {
    controller.cleanup();
	render_scene.cleanup(*game.renderer.global_allocator);
    game.scenes.remove_value(this);
}



void Scene::inspect_entity(entt::entity entity) {
    ZoneScoped;
	if (!registry.any_of<Name>(entity))
		return;

	auto& name = registry.get<Name>(entity).name;

	if (name.empty())
		name = "empty";
	if (ImGui::TreeNode(name.c_str())) {
		ImGui::PushID((u32) entity);

        ImGui::Text("ID: %d", (u32) entity);

	    inspect_components(this, entity);

		ImGui::PopID();
		ImGui::TreePop();
	}
}

void Scene::settings_window(bool* p_open) {
    ZoneScoped;
	if (ImGui::Begin((name + " Info").c_str(), p_open)) {
		ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
		if (ImGui::BeginTabBar("SceneTabs", tab_bar_flags)) {
			if (ImGui::BeginTabItem("Entities")) {
				for (auto entity : registry.view<Name>())
					inspect_entity(entity);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Camera")) {
				inspect(&camera);
				ImGui::Separator();
				ImGui::Text("Controller");
				ImGui::Indent();
				inspect(&controller);
				ImGui::Unindent();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Render Scene")) {
			    render_scene.settings_gui();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	ImGui::End();
}

void Scene::output_window(bool* p_open) {
    ZoneScoped;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
    if (ImGui::Begin((name + " Output").c_str(), p_open)) {
        render_scene.viewport.window_hovered = ImGui::IsWindowHovered();
        render_scene.image((v2i) ImGui::GetContentRegionAvail());
        render_scene.cull_pause = false;
    } else {
        render_scene.viewport.window_hovered = false;
        render_scene.cull_pause = true;
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

entt::entity Scene::get_lizard(v3i pos) {
    auto view = registry.view<LogicTransform, Lizard>();
    for (auto [entity, logic_pos, lizard] : view.each()) {
        if (math::length(logic_pos.position - v3(pos)) < 0.1f) {
            return entity;
        }
    }
    return entt::null;
}

entt::entity Scene::get_tile(v3i pos) {
    auto view = registry.view<LogicTransform, GridSlot>();
    for (auto [entity, logic_pos, tile] : view.each()) {
        if (math::length(logic_pos.position - v3(pos)) < 0.1f) {
            return entity;
        }
    }
    return entt::null;
}

vector<entt::entity> Scene::get_enemies(v3i pos) {
    vector<entt::entity> entities;
    auto view = registry.view<LogicTransform, Traveler>();
    for (auto [entity, logic_pos, traveler] : view.each()) {
        if (math::length(logic_pos.position - v3(pos)) < 0.1f) {
            entities.push_back(entity);
        }
    }
    return entities;
}

entt::entity Scene::get_spawner(v3i pos) {
    auto view = registry.view<LogicTransform, Spawner>();
    for (auto [entity, logic_pos, spawner] : view.each()) {
        if (math::length(logic_pos.position - v3(pos)) < 0.1f) {
            return entity;
        }
    }
    return entt::null;
}

entt::entity Scene::get_consumer(v3i pos) {
    auto view = registry.view<LogicTransform, Consumer>();
    for (auto [entity, logic_pos, traveler] : view.each()) {
        if (math::length(logic_pos.position - v3(pos)) < 0.1f) {
            return entity;
        }
    }
    return entt::null;
}

vector<entt::entity> Scene::get_any(v3i pos) {
    vector<entt::entity> entities;
    auto view = registry.view<LogicTransform>();
    for (auto [entity, logic_pos] : view.each()) {
        if (math::length(logic_pos.position - v3(pos)) < 0.1f) {
            entities.push_back(entity);
        }
    }
    return entities;
}

entt::entity Scene::get(v3i pos, LizardPrefab* t) {
    return get_lizard(pos);
}

entt::entity Scene::get(v3i pos, TilePrefab* t) {
    return get_tile(pos);
}

entt::entity Scene::get(v3i pos, SpawnerPrefab* t) {
    return get_spawner(pos);
}

entt::entity Scene::get(v3i pos, ConsumerPrefab* t) {
    return get_consumer(pos);
}


}
