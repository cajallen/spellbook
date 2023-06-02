#include "scene.hpp"

#include <tracy/Tracy.hpp>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>

#include "extension/fmt.hpp"
#include "extension/fmt_geometry.hpp"
#include "general/math.hpp"
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
#include "game/entities/impair.hpp"
#include "game/entities/projectile.hpp"
#include "general/astar.hpp"

namespace spellbook {

Scene::Scene() {}
Scene::~Scene() {}


void Scene::model_cleanup(entt::registry& registry, entt::entity entity) {
    Model& model = registry.get<Model>(entity);
    deinstance_model(render_scene, model.model_gpu);
}

void Scene::dragging_setup(entt::registry& registry, entt::entity entity) {
    auto impairs = registry.try_get<Impairs>(entity);
    if (impairs) {
        apply_untimed_impair(*impairs, Dragging::magic_number | u32(entity), ImpairType_NoCast);
    }

    auto caster = registry.try_get<Caster>(entity);
    if (caster->casting()) {
        caster->attack->stop_casting();
        caster->ability->stop_casting();
    }
}

void Scene::dragging_cleanup(entt::registry& registry, entt::entity entity) {
    auto&    dragging = registry.get<Dragging>(entity);
    auto& logic_tfm = registry.get<LogicTransform>(entity);
    if (math::length(logic_tfm.position - math::round(dragging.potential_logic_position)) > 0.1f)
        player.bank.beads[Bead_Quartz]--;
    
    logic_tfm.position = math::round(dragging.potential_logic_position);

    constexpr float drag_fade_duration = 0.15f;
    
    auto poser = registry.try_get<PoseController>(entity);
    if (poser) {
        poser->set_state(AnimationState_Idle, 0.0f, drag_fade_duration);
    }

    bool remain_impaired = false;
    if (is_casting_platform(math::round_cast(dragging.potential_logic_position))) {
        logic_tfm.position += v3(0.0f, 0.0, 0.1f);

        auto caster = registry.try_get<Caster>(entity);
        if (caster) {
            Ability* ability = &*caster->ability;
            add_timer(caster->ability->scene, "drag_cast_delay",
                [ability](Timer* timer) {
                    ability->targeting();
                    ability->request_cast();
                }, true
            )->start(drag_fade_duration);
            remain_impaired = true;
        }
    }

    if (!remain_impaired) {
        remove_dragging_impair(registry, entity);
    }
}

void Scene::health_cleanup(entt::registry& registry, entt::entity entity) {
    auto& health = registry.get<Health>(entity);
}

void Scene::caster_cleanup(entt::registry& registry, entt::entity entity) {
    auto& caster = registry.get<Caster>(entity);
}


void Scene::emitter_cleanup(entt::registry& registry, entt::entity entity) {
    auto& emitter = registry.get<EmitterComponent>(entity);
    for (auto& [id, emitter_gpu] : emitter.emitters)
        deinstance_emitter(*emitter_gpu, true);
}

void Scene::gridslot_cleanup(entt::registry& registry, entt::entity entity) {
    GridSlot* grid_slot = registry.try_get<GridSlot>(entity);
    if (!grid_slot)
        return;
    for (entt::entity neighbor : grid_slot->linked) {
        registry.get<GridSlot>(neighbor).linked.clear();
        if (registry.valid(neighbor))
            registry.destroy(neighbor);
    }
}

void Scene::setup(const string& input_name) {
    name = input_name;
	render_scene.name = name + "::render_scene";
	camera = Camera(v3(-8.0f, 0.0f, 4.0f), math::d2r(euler{0.0f, -30.0f}));
	render_scene.viewport.name	 = render_scene.name + "::viewport";
	render_scene.viewport.camera = &camera;
	render_scene.viewport.setup();
	controller.name = name + "::controller";
	controller.setup(&render_scene.viewport, &camera);

    registry.on_construct<Dragging>().connect<&on_dragging_create>(*this);
    registry.on_destroy<Model>().connect<&on_model_destroy>(*this);
    registry.on_destroy<Dragging>().connect<&on_dragging_destroy>(*this);
    registry.on_destroy<EmitterComponent>().connect<&on_emitter_component_destroy>(*this);
    registry.on_destroy<GridSlot>().connect<&on_gridslot_destroy>(*this);
    registry.on_destroy<Enemy>().connect<&on_enemy_destroy>(*this);

    game.scenes.push_back(this);
	game.renderer.add_scene(&render_scene);

    spawn_state_info = new SpawnStateInfo();
    player.bank.beads[Bead_Quartz] = 2;

    targeting = std::make_unique<MapTargeting>();
    targeting->scene = this;

    navigation = std::make_unique<astar::Navigation>(&map_data.path_solids, &map_data.slot_solids, &map_data.unstandable_solids, &map_data.ramps);
}

void Scene::update() {
	ZoneScoped;
    
    delta_time = Input::delta_time * time_scale;
    frame++;
    time += delta_time;

    map_data.clear();
    for (auto [entity, slot, logic_tfm] : registry.view<GridSlot, LogicTransform>().each()) {
        if (slot.ramp) {
            map_data.ramps[v3i(logic_tfm.position)] = slot.direction;
            continue;
        }
        if (!slot.standable)
            map_data.unstandable_solids.set(v3i(logic_tfm.position));
        else if (slot.path)
            map_data.path_solids.set(v3i(logic_tfm.position));
        else
            map_data.slot_solids.set(v3i(logic_tfm.position));
        map_data.solids.set(v3i(logic_tfm.position));
    }
    paths.clear();
    for (auto [spawner_e, spawner, spawner_tfm] : registry.view<Spawner, LogicTransform>().each()) {
        for (auto [consumer_e, consumer, consumer_tfm] : registry.view<Consumer, LogicTransform>().each()) {
            vector<v3> path = navigation->find_path(math::round_cast(spawner_tfm.position), math::round_cast(consumer_tfm.position));
            if (math::distance(path.front(), consumer_tfm.position) < 0.1f) {
                paths.emplace_back(spawner_e, consumer_e, std::move(path));
            }
        }
    }
    for (auto& path : paths) {
        vector<FormattedVertex> vertices;
        float x = time * 6.0f;
        for (v3& v : path.path) {
            float hue = 190.0f + 10.0f * math::sin(x);
            float saturation = 0.3f + 0.1f * math::sin(x);
            float width = 0.03f - 0.01f * math::clamp(math::sin(x), -1.0f, 0.0f);
            vertices.emplace_back(v + v3(0.5f, 0.5f, 0.05f), Color::hsv(hue, saturation, 0.8f), width);
            x += 1.5f;
        }
        render_scene.quick_mesh(generate_formatted_line(render_scene.viewport.camera, std::move(vertices)), true, true);
    }
    
    
    update_timers(this);
    
	controller.update();

    dragging_update_system(this);
    collision_update_system(this);
    enemy_aggro_system(this);
    dragging_system(this);
    spawner_system(this);
    travel_system(this);
    area_trigger_system(this);
    enemy_ik_controller_system(this);

    caster_system(this);
    projectile_system(this);

    pickup_system(this);
    
    visual_tile_widget_system(this);
    spawner_draw_system(this);
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

void Scene::set_edit_mode(bool to) {
    render_scene.render_grid = false;
    render_scene.render_widgets = to;
    edit_mode = to;
}

void Scene::cleanup() {
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
        ImGui::SliderFloat("Time Scale", &time_scale, 0.01f, 20.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
	    
		ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
		if (ImGui::BeginTabBar("SceneTabs", tab_bar_flags)) {
			if (ImGui::BeginTabItem("Entities")) {
			    auto named_entities = registry.view<Name>();
				for (auto it = named_entities.rbegin(), last = named_entities.rend(); it != last; ++it)
				    if (!registry.any_of<AddToInspect>(*it))
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

entt::entity Scene::get_consumer(v3i pos) {
    auto view = registry.view<LogicTransform, Consumer>();
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
    // emitter_cpu.position = position;
    auto e = scene->registry.create();
    
    struct EmitterTimerTimeoutPayload {
        Scene* scene;
        entt::entity entity;
    };

    auto payload = new EmitterTimerTimeoutPayload(scene, e);
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

    Impairs* impairs = registry.try_get<Impairs>(selected_entity);
    if (impairs)
        if (impairs->is_impaired(this, ImpairType_NoMove))
            return;
    
    if (registry.all_of<Draggable>(selected_entity) && player.bank.beads[Bead_Quartz] > 0) {
        registry.emplace<Dragging>(selected_entity, time, logic_tfm->position, intersect);
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
