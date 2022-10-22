#include "scene.hpp"

#include <tracy/Tracy.hpp>

#include "lib_ext/fmt_geometry.hpp"

#include "math.hpp"
#include "game.hpp"

#include "game/components.hpp"
#include "game/systems.hpp"
#include "renderer/draw_functions.hpp"

namespace spellbook {

void Scene::model_cleanup(entt::registry& registry, entt::entity entity) {
    Model& model = registry.get<Model>(entity);
    // TODO:
}
void Scene::dragging_cleanup(entt::registry& registry, entt::entity entity) {
    auto transform = registry.try_get<Transform>(entity);
    if (transform) {
        transform->translation = math::round(transform->translation);
        transform->translation.z = 0.0f;
    }
}



void Scene::setup() {
	render_scene.name = name + "::render_scene";
	// setup camera
	cameras.emplace_back(v3(-8, 0, 4), math::d2r(euler{0, -30}));
	render_scene.viewport.name	 = render_scene.name + "::viewport";
	render_scene.viewport.camera = &cameras.last();
	render_scene.viewport.setup();
	controller.name = name + "::controller";
	controller.setup(&render_scene.viewport, &cameras.last());

	game.renderer.add_scene(&render_scene);

    // TODO: ship entity

    registry.on_destroy<Model>().connect<&Scene::model_cleanup>(*this);
    registry.on_destroy<Dragging>().connect<&Scene::dragging_cleanup>(*this);
}

#define PREVIEW_COMPONENT(Component, component) if (auto* component = registry.try_get<Component>(entity)) { component->preview_3d(this, entity); }

void Scene::update() {
	ZoneScoped;
	controller.update();

    dragging_update_system(this);
    collision_update_system(this);
    dragging_system(this);
    spawner_system(this);
    travel_system(this);
    pyro_system(this);
    roller_system(this);
    rollee_system(this);
    grid_slot_transform_system(this);
    transform_system(this);
    selection_id_system(this);
    consumer_system(this);
    health_system(this);
    health_draw_system(this);
    disposal_system(this);

    for (auto entity : registry.view<Name>()) {
        PREVIEW_COMPONENT(Model, model)
        PREVIEW_COMPONENT(Transform, transform)
        PREVIEW_COMPONENT(GridSlot, grid_slot)
        PREVIEW_COMPONENT(Traveler, traveler)
        PREVIEW_COMPONENT(Health, health)
        PREVIEW_COMPONENT(Spawner, spawner)
        PREVIEW_COMPONENT(Consumer, consumer)
        PREVIEW_COMPONENT(Killed, killed)
        PREVIEW_COMPONENT(Pyro, pyro)
        PREVIEW_COMPONENT(Roller, roller)
        PREVIEW_COMPONENT(Rollee, rollee)
        PREVIEW_COMPONENT(Dragging, dragging)
        PREVIEW_COMPONENT(Collision, collision)
    }

}

void Scene::cleanup() {
	render_scene.cleanup(*game.renderer.global_allocator);
    // TODO: should remove render scene from render scenes list
}


#define INSPECT_COMPONENT(Component, component) if (auto* component = registry.try_get<Component>(entity)) { if (ImGui::TreeNode(#Component)) { component->inspect(this); ImGui::TreePop(); } }

void Scene::inspect_entity(entt::entity entity) {
	if (!registry.any_of<Name>(entity))
		return;

	auto& name = registry.get<Name>(entity).name;

	if (name == "")
		name = "empty";
	if (ImGui::TreeNode(name.c_str())) {
		ImGui::PushID((u32) entity);

        ImGui::Text("ID: %d", (u32) entity);

        INSPECT_COMPONENT(Model, model)
        INSPECT_COMPONENT(Transform, transform)
        INSPECT_COMPONENT(GridSlot, grid_slot)
        INSPECT_COMPONENT(Traveler, traveler)
        INSPECT_COMPONENT(Health, health)
        INSPECT_COMPONENT(Spawner, spawner)
        INSPECT_COMPONENT(Consumer, consumer)
        INSPECT_COMPONENT(Killed, killed)
        INSPECT_COMPONENT(Pyro, pyro)
        INSPECT_COMPONENT(Roller, roller)
        INSPECT_COMPONENT(Rollee, rollee)
        INSPECT_COMPONENT(Dragging, dragging)
        INSPECT_COMPONENT(Collision, collision)

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
				ImGui::Text("Cameras");
				ImGui::Indent();
				for (Camera& camera : cameras)
					inspect(&camera);
				ImGui::Unindent();
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
    } else {
        render_scene.viewport.window_hovered = false;
        render_scene.pause = true;
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

}
