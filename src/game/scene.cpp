#include "scene.hpp"

#include <tracy/Tracy.hpp>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>
#include <entt/core/hashed_string.hpp>

#include "extension/fmt.hpp"
#include "extension/fmt_geometry.hpp"
#include "general/math/math.hpp"
#include "general/bitmask_3d.hpp"
#include "renderer/draw_functions.hpp"
#include "editor/console.hpp"
#include "entities/area_trigger.hpp"
#include "entities/caster.hpp"
#include "game/game.hpp"
#include "game/systems.hpp"
#include "game/pose_controller.hpp"
#include "game/entities/components.hpp"
#include "game/entities/spawner.hpp"
#include "game/entities/consumer.hpp"
#include "game/entities/tile.hpp"
#include "game/entities/enemy.hpp"
#include "game/entities/tags.hpp"
#include "game/entities/projectile.hpp"
#include "general/astar.hpp"

namespace spellbook {

using namespace entt::literals;

Scene::Scene() {}
Scene::~Scene() {}

void Scene::setup(const string& input_name) {
    name = input_name;
	render_scene.name = name + "::render_scene";
	camera = Camera(v3(-8.0f, 8.0f, 6.0f), math::d2r(euler{-45.0f, -30.0f}));
	render_scene.viewport.name	 = render_scene.name + "::viewport";
	render_scene.viewport.camera = &camera;
	render_scene.viewport.setup();
	controller.name = name + "::controller";
	controller.setup(&render_scene.viewport, &camera);

    registry.on_construct<Dragging>().connect<&on_dragging_create>(*this);
    registry.on_destroy<Model>().connect<&on_model_destroy>(*this);
    registry.on_destroy<StaticModel>().connect<&on_model_destroy>(*this);
    registry.on_destroy<Dragging>().connect<&on_dragging_destroy>(*this);
    registry.on_destroy<EmitterComponent>().connect<&on_emitter_component_destroy>(*this);
    registry.on_destroy<GridSlot>().connect<&on_gridslot_destroy>(*this);
    registry.on_destroy<Enemy>().connect<&on_enemy_destroy>(*this);
    registry.on_construct<GridSlot>().connect<&on_gridslot_create>(*this);
    registry.on_destroy<GridSlot>().connect<&on_gridslot_destroy>(*this);

    game.scenes.push_back(this);
	game.renderer.add_scene(&render_scene);

    spawn_state_info = new SpawnStateInfo();
    player.bank.beads[Bead_Quartz] = 2;

    targeting = std::make_unique<MapTargeting>();
    targeting->scene = this;

    navigation = std::make_unique<astar::Navigation>(&map_data.path_solids, &map_data.slot_solids, &map_data.unstandable_solids, &map_data.ramps);

    audio.setup();
}

void Scene::update() {
	ZoneScoped;
    
    frame++;
    delta_time = Input::delta_time * time_scale;
    time += delta_time;

    if (map_data.dirty)
        map_data.update(registry);
    update_paths(paths, *this);
    
    update_timers(this);
    
	controller.update();
    audio.update(this);

    traveler_reset_system(this);
    
    dragging_update_system(this);
    collision_update_system(this);
    enemy_aggro_system(this);
    dragging_system(this);
    spawner_system(this);
    travel_system(this);
    enemy_decollision_system(this);
    scene_vertical_offset_system(this);
    area_trigger_system(this);
    enemy_ik_controller_system(this);
    attachment_transform_system(this);

    caster_system(this);
    projectile_system(this);

    pickup_system(this);
    
    visual_tile_widget_system(this);
    transform_system(this);
    emitter_system(this);
    pose_system(this);
    selection_id_system(this);
    consumer_system(this);
    health_system(this);
    health_draw_system(this);
    disposal_system(this);


    for (auto entity : registry.view<Name>()) {
        preview_3d_components(this, entity);
    }
}

void MapData::update(entt::registry& registry) {
    ZoneScoped;
    clear();
    for (auto [entity, slot, logic_tfm] : registry.view<GridSlot, LogicTransform>().each()) {
        if (slot.ramp) {
            ramps[v3i(logic_tfm.position)] = slot.direction;
            continue;
        }
        if (!slot.standable)
            unstandable_solids.set(v3i(logic_tfm.position));
        else if (slot.path)
            path_solids.set(v3i(logic_tfm.position));
        else
            slot_solids.set(v3i(logic_tfm.position));
        solids.set(v3i(logic_tfm.position));
    }
    dirty = false;
}
    
void MapData::clear() {
    solids.clear();
    path_solids.clear();
    slot_solids.clear();
    unstandable_solids.clear();
    ramps.clear();
}

void update_paths(vector<PathInfo>& paths, Scene& scene) {
    ZoneScoped;
    paths.clear();
    for (auto [spawner_e, spawner, spawner_tfm] : scene.registry.view<Spawner, LogicTransform>().each()) {
        for (auto [consumer_e, consumer, consumer_tfm] : scene.registry.view<Shrine, LogicTransform>().each()) {
            Path path = scene.navigation->find_path(math::round_cast(spawner_tfm.position), math::round_cast(consumer_tfm.position));
            if (math::distance(path.get_destination(), consumer_tfm.position) < 0.1f) {
                paths.emplace_back(spawner_e, consumer_e, std::move(path));
            }
        }
    }
}

void render_paths(vector<PathInfo>& paths, RenderScene& render_scene, float time) {
    ZoneScoped;
    for (auto& path_info : paths) {
        vector<FormattedVertex> vertices;
        float x = time * 6.0f;
        for (v3& v : path_info.path.waypoints) {
            float hue = 190.0f + 10.0f * math::sin(x);
            float saturation = 0.3f + 0.1f * math::sin(x);
            float width = 0.03f - 0.01f * math::clamp(math::sin(x), -1.0f, 0.0f);
            vertices.emplace_back(v + v3(0.5f, 0.5f, 0.05f), Color::hsv(hue, saturation, 0.8f), width);
            x += 1.5f;
        }
        render_scene.quick_mesh(generate_formatted_line(render_scene.viewport.camera, std::move(vertices)), true, false);
    }
}


void Scene::set_edit_mode(bool to) {
    render_scene.render_grid = false;
    render_scene.render_widgets = to;
    edit_mode = to;
}

void Scene::cleanup() {
    audio.shutdown();
    delete spawn_state_info;
    
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
		ImGui::PushID((uint32) entity);

        ImGui::Text("ID: %d", (uint32) entity);

	    inspect_components(this, entity);

		ImGui::PopID();
		ImGui::TreePop();
	}
}

void Scene::settings_window(bool* p_open) {
    ZoneScoped;
	if (ImGui::Begin((name + " Info").c_str(), p_open)) {
        ImGui::SliderFloat("Time Scale", &time_scale, 0.01f, 20.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
	    
		ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
		if (ImGui::BeginTabBar("SceneTabs", tab_bar_flags)) {
			if (ImGui::BeginTabItem("Entities")) {
			    auto named_entities = registry.view<AddToInspect>();
				for (auto it = named_entities.rbegin(), last = named_entities.rend(); it != last; ++it)
                    inspect_entity(*it);

			    ImGui::Dummy(ImVec2{1.0f, 400.0f});
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
    
    if (!edit_mode) {
        if (ImGui::Begin((name + " Shop").c_str())) {
            inspect(spawn_state_info);
            if (ImGui::BeginTabBar("ShopTabBar")) {
                if (ImGui::BeginTabItem("Shop")) {
                    show_shop(&shop, &player);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Generator")) {
                    shop.shop_generator->inspect();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();
    }
}

void Scene::output_window(bool* p_open) {
    ZoneScoped;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0, 0});
    if (ImGui::Begin((name + " Output").c_str(), p_open)) {
        pause = false;
        render_scene.viewport.window_hovered = ImGui::IsWindowHovered();
        render_scene.image((v2i) ImGui::GetContentRegionAvail());
        // if (ImGui::BeginDragDropTarget()) {
        //     if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("DND_ENTITY_LIZARD")) {
        //         
        //     }
        //     ImGui::EndDragDropTarget();
        // }
        render_scene.cull_pause = false;
    } else {
        pause = true;
        render_scene.viewport.window_hovered = false;
        render_scene.cull_pause = true;
    }
    ImGui::End();
    ImGui::PopStyleVar();
}


bool Scene::is_casting_platform(v3i pos) {
    auto view = registry.view<LogicTransform, CastingPlatform>();
    for (auto [entity, logic_tfm, platform] : view.each()) {
        if (math::length(logic_tfm.position - v3(pos)) < 0.5f) {
            return true;
        }
    }
    return false;
}

entt::entity Scene::get_tile(v3i pos) {
    auto view = registry.view<LogicTransform, GridSlot>();
    for (auto [entity, logic_tfm, tile] : view.each()) {
        if (math::length(logic_tfm.position - v3(pos)) < 0.1f) {
            return entity;
        }
    }
    return entt::null;
}

entt::entity Scene::get_spawner(v3i pos) {
    auto view = registry.view<LogicTransform, Spawner>();
    for (auto [entity, logic_tfm, spawner] : view.each()) {
        if (math::length(logic_tfm.position - v3(pos)) < 0.1f) {
            return entity;
        }
    }
    return entt::null;
}

entt::entity Scene::get_shrine(v3i pos) {
    auto view = registry.view<LogicTransform, Shrine>();
    for (auto [entity, logic_tfm, traveler] : view.each()) {
        if (math::length(logic_tfm.position - v3(pos)) < 0.1f) {
            return entity;
        }
    }
    return entt::null;
}

vector<entt::entity> Scene::get_any(v3i pos) {
    vector<entt::entity> entities;
    auto view = registry.view<LogicTransform>();
    for (auto [entity, logic_tfm] : view.each()) {
        if (math::length(logic_tfm.position - v3(pos)) < 0.1f) {
            entities.push_back(entity);
        }
    }
    return entities;
}

entt::entity quick_emitter(Scene* scene, const string& name, v3 position, const string& emitter_path, float duration) {
    return quick_emitter(scene, name, position, load_asset<EmitterCPU>(emitter_path), duration);
}

entt::entity quick_emitter(Scene* scene, const string& name, v3 position, EmitterCPU emitter_cpu, float duration) {
    auto e = scene->registry.create();
    
    struct EmitterTimerTimeoutPayload {
        Scene* scene;
        entt::entity entity;
    };
    
    scene->registry.emplace<Name>(e, name);
    scene->registry.emplace<LogicTransform>(e, position);
    
    auto& emitter_comp = scene->registry.emplace<EmitterComponent>(e, scene);
    // we destroy all emitters on death, so we don't need ids
    emitter_comp.add_emitter(0, emitter_cpu);
    
    add_timer(scene, fmt_("{}_timer", name), [scene, e](Timer* timer) {
        scene->registry.destroy(e);
    }, true)->start(duration);
    
    return e;
}

void Scene::select_entity(entt::entity entity) {
    selected_entity = entity;
    v3  intersect = math::intersect_axis_plane(render_scene.viewport.ray((v2i) Input::mouse_pos), Z, 0.0f);
    LogicTransform* logic_tfm = registry.try_get<LogicTransform>(selected_entity);
    if (!logic_tfm)
        return;

    Tags* tags = registry.try_get<Tags>(selected_entity);
    if (tags)
        if (tags->has_tag("no_move"_hs))
            return;
    
    if (registry.all_of<Draggable>(selected_entity) && player.bank.beads[Bead_Quartz] > 0) {
        Draggable& draggable = registry.get<Draggable>(selected_entity);
        registry.emplace<Dragging>(selected_entity, draggable.drag_height, time, logic_tfm->position, intersect);
    }
}

bool Scene::get_object_placement(v2i offset, v3i& pos) {
    ray3 mouse_ray = render_scene.viewport.ray(math::round_cast(Input::mouse_pos) + offset);
    v3 intersect;
    return ray_intersection(map_data.solids, mouse_ray, intersect, pos, {});
}

bool Scene::get_object_placement(v3i& pos) {
    return get_object_placement(v2i{}, pos);
}



}
