#include "systems.hpp"

#include <tracy/Tracy.hpp>
#include <entt/entt.hpp>

#include "extension/fmt.hpp"
#include "extension/fmt_geometry.hpp"
#include "general/astar.hpp"
#include "general/matrix_math.hpp"
#include "general/logger.hpp"
#include "game/game.hpp"
#include "game/scene.hpp"
#include "game/components.hpp"
#include "game/input.hpp"
#include "editor/console.hpp"
#include "renderer/render_scene.hpp"
#include "renderer/draw_functions.hpp"

namespace spellbook {

void travel_system(Scene* scene) {
    ZoneScoped;
    // construct grid to use for pathfinding
    auto slots     = scene->registry.view<GridSlot, LogicTransform>();
    auto entities  = scene->registry.view<Traveler, LogicTransform>();
    auto consumers = scene->registry.view<Consumer>();

    astar::Navigation nav;
    for (auto [entity, slot, logic_pos] : slots.each()) {
        if (slot.path)
            nav.positions.push_back(v2i(logic_pos.position.xy));
    }

    // handle actual traveling
    for (auto [entity, traveler, transform] : entities.each()) {
        bool has_path = !traveler.targets.empty();

        if (!has_path && !consumers.empty()) {
            int       random_consumer = math::random_s32() % s32(consumers.size());
            auto      consumer_entity = consumers[random_consumer];
            LogicTransform* p_logic_position          = scene->registry.try_get<LogicTransform>(consumer_entity);
            assert_else(p_logic_position);

            auto path = nav.find_path(math::round_cast(transform.position.xy), v2i(p_logic_position->position.xy));
            for (auto it = path.begin(); it != path.end(); ++it) {
                traveler.targets.emplace_back(it->x, it->y, 0);
            }
        }
        assert_else(!math::is_nan(transform.position.x)) {
            transform.position = v3(0, 0, 0);
        }

        v3i  target_position = has_path ? traveler.targets.back() : math::round_cast(transform.position);
        v3   velocity        = v3(target_position) - transform.position;
        bool at_target       = math::length(velocity) < 0.01f;
        if (at_target && has_path) {
            traveler.targets.remove_back();
            velocity = v3(0);
        }
        f32 max_velocity = traveler.max_speed.value() * scene->delta_time;
        f32 min_velocity = 0.0f;
        if (!at_target)
            transform.position += math::normalize(velocity) * math::clamp(math::length(velocity), min_velocity, max_velocity);
    }
}

// Creates the frame renderable for health bars
void health_draw_system(Scene* scene) {
    ZoneScoped;

    if (scene->edit_mode)
        return;
    
    // dependencies
    static bool deps = false;
    static string mesh_name;
    static string health_name;
    static string health_bar_name;
    if (!deps) {
        mesh_name = upload_mesh(generate_cube(v3(0), v3(1)));
        
        MaterialCPU material1_cpu = {
            .file_path     = "health_material",
            .color_tint    = palette::black,
            .emissive_tint = palette::green
        };
        health_name = upload_material(material1_cpu);
        MaterialCPU material2_cpu = {
            .file_path     = "health_bar_material",
            .color_tint    = palette::black,
            .cull_mode     = vuk::CullModeFlagBits::eFront
        };
        health_bar_name = upload_material(material2_cpu);
        deps = true;
    }

    auto       health_draw_system = scene->registry.view<LogicTransform, Health>();
    for (auto [entity, transform, health] : health_draw_system.each()) {
        if (health.value <= 0.0f)
            continue;

        float percentage = health.value / health.max_health.value();
        auto link = scene->registry.try_get<TransformLink>(entity);
        v3 position = link ? transform.position + link->offset : transform.position;

        float dir_to_camera = math::angle_to(scene->camera.position.xy, position.xy);
        
        constexpr float gap = 0.02f;
        constexpr float thickness = 0.03f;
        constexpr float width = 0.6f;
        m44 inner_matrix = math::translate(position) * 
                           math::rotation(euler{dir_to_camera - math::PI / 2.0f, 0.0f}) *
                           math::translate(v3(-(1.0f - percentage) * 0.5f * width, 0.0f, 0.5f)) *
                           math::scale(v3(percentage * 0.5f * width, thickness, thickness));
        m44 outer_matrix = math::translate(v3(0.0f, 0.0f, 0.5) + position) *
                           math::rotation(euler{dir_to_camera - math::PI * 0.5f, 0.0f}) * 
                           math::scale(v3(0.5f * width + gap, thickness + gap, thickness + gap));

        auto renderable1 = Renderable{mesh_name, health_name, (m44GPU) inner_matrix, {}, true};
        auto renderable2 = Renderable{mesh_name, health_bar_name, (m44GPU) outer_matrix, {}, true};
        scene->render_scene.add_renderable(renderable1);
        scene->render_scene.add_renderable(renderable2);
    }
}

// Uses the transform for models
void transform_system(Scene* scene) {
    auto& registry = scene->registry;
    ZoneScoped;
    // Logic Transform Attach
    for (auto [entity, attach, transform] : registry.view<LogicTransformAttach, LogicTransform>().each()) {
        // We disable attachments for dragging
        if (registry.any_of<Dragging>(entity))
            continue;
        
        if (registry.valid(attach.to)) {
            transform = registry.get<LogicTransform>(attach.to);
        } else {
            console_error("Invalid attachment", "components.transform", ErrorType_Warning);
            registry.erase<LogicTransformAttach>(entity);
        }
    }
    // Transform Link
    for (auto [entity, link, l_transform, m_transform] : registry.view<TransformLink, LogicTransform, ModelTransform>().each()) {
        // We disable links for dragging
        if (registry.any_of<Dragging>(entity))
            continue;
        
        m_transform.translation = l_transform.position + link.offset;
    }
    // Apply transforms to renderables
    for (auto [entity, model, transform] : registry.view<Model, ModelTransform>().each()) {
        m44 transform_matrix = math::translate(transform.translation) * math::rotation(transform.rotation) * math::scale(transform.scale);
        for (auto& [node, renderable] : model.model_gpu.renderables) {
            renderable->transform = (m44GPU) (transform_matrix * node->calculate_transform());
        }
    }
}

void consumer_system(Scene* scene) {
    ZoneScoped;
    auto consumers = scene->registry.view<Consumer, LogicTransform>();
    auto consumees = scene->registry.view<Traveler, LogicTransform>();

    for (auto [e_consumer, consumer, consumer_transform] : consumers.each()) {
        auto consumer_mtransform = scene->registry.try_get<ModelTransform>(e_consumer);
        if (consumer_mtransform) {
            consumer_mtransform->rotation.yaw += (scene->time_scale == 0.0f ? Input::delta_time : scene->delta_time) * math::TAU / 6.f;
            consumer_mtransform->rotation.pitch += (scene->time_scale == 0.0f ? Input::delta_time : scene->delta_time) * math::TAU / 11.f;
            consumer_mtransform->rotation.roll += (scene->time_scale == 0.0f ? Input::delta_time : scene->delta_time) * math::TAU / 19.f;
        }
        for (auto [e_consumee, consumee, consumee_transform] : consumees.each()) {
            if (scene->registry.any_of<Killed>(e_consumee))
                continue;
            f32 dist = math::length(v2(consumer_transform.position.xy) - consumee_transform.position.xy);
            if (dist < consumer.consume_distance && !scene->edit_mode) {
                scene->registry.emplace<Killed>(e_consumee, scene->time);
            }
        }
    }
}

void disposal_system(Scene* scene) {
    ZoneScoped;
    if (scene->edit_mode)
        return;
    
    auto killed = scene->registry.view<Killed>();
    scene->registry.destroy(killed.begin(), killed.end());
}

void health_system(Scene* scene) {
    ZoneScoped;
    auto healths = scene->registry.view<Health>();
    for (auto [entity, health] : healths.each()) {
        if (health.value <= 0.001f) {
            scene->registry.emplace<Killed>(entity, scene->time);
        }
    }
}

void selection_id_system(Scene* scene) {
    ZoneScoped;
    auto model_view = scene->registry.view<Model>();
    for (auto [entity, model] : model_view.each()) {
        u32 id = (u32) entity;
        auto attach = scene->registry.try_get<LogicTransformAttach>(entity);
        if (attach != nullptr)
            id = (u32) attach->to;
        for (auto [_, r] : model.model_gpu.renderables) {
            r->selection_id = id;
        }
    }
}

void skeleton_system(Scene* scene) {
    ZoneScoped;
    auto model_view = scene->registry.view<Model>();
    for (auto [entity, model] : model_view.each()) {
        for (auto& skeleton : model.model_gpu.skeletons) {
            skeleton->update();
        }
    }
}

void pose_system(Scene* scene) {
    ZoneScoped;
    auto model_view = scene->registry.view<Model, PoseController>();
    for (auto [entity, model, poser] : model_view.each()) {
        for (auto& skeleton : model.model_gpu.skeletons) {
            auto& skeleton_cpu = *skeleton->skeleton_cpu;
            skeleton_cpu.time += Input::delta_time;
            auto& poses = skeleton_cpu.poses;
            if (poser.target_state == "flail") {
                if (poses.contains("flail_1") && poses.contains("flail_2")) {
                    if (math::mod(skeleton_cpu.time, 0.5f) < 0.25f) {
                        if (skeleton_cpu.current_pose != "flail_1")
                            skeleton_cpu.load_pose("flail_1", true, 0.25f);
                    } else {
                        if (skeleton_cpu.current_pose != "flail_2") {
                            skeleton_cpu.load_pose("flail_2", true, 0.25f);
                        }
                    }
                }
                else {
                    log_warning("Pose doesn't support flail");
                }
            }
            if (poser.target_state == "default") {
                if (poses.contains("default")) {
                    if (skeleton_cpu.current_pose != "default")
                        skeleton_cpu.load_pose("default");
                }
                else {
                    log_warning("Pose doesn't support default");
                }
            }
        }
    }
}


// Adds dragging to entities
void dragging_update_system(Scene* scene) {
    ZoneScoped;
    if (scene->render_scene.fut_query_result.get_control()) {
        vuk::Compiler compiler;
        auto result_buf = *scene->render_scene.fut_query_result.get<vuk::Buffer>(*game.renderer.global_allocator, compiler);
        u32 result_int = *((u32*) result_buf.mapped_ptr);
        scene->render_scene.fut_query_result = {};

        if (scene->registry.valid((entt::entity) result_int)) {
            scene->selected_entity = (entt::entity) result_int;
            v3  intersect = math::intersect_axis_plane(scene->render_scene.viewport.ray((v2i) Input::mouse_pos), Z, 0.0f);
            auto transform = scene->registry.try_get<LogicTransform>(scene->selected_entity);
            if (!transform)
                return;

            scene->registry.emplace<Dragging>(scene->selected_entity, scene->time, transform->position, intersect);

            for (auto [entity, attach] : scene->registry.view<LogicTransformAttach>().each()) {
                if (attach.to == scene->selected_entity) {
                    auto attachee_transform = scene->registry.try_get<LogicTransform>(entity);
                    scene->registry.emplace<Dragging>(entity, scene->time, attachee_transform->position, intersect);
                }
            }
        }
    }
}

void dragging_system(Scene* scene) {
    ZoneScoped;
    Viewport& viewport = scene->render_scene.viewport;
    v3 intersect = math::intersect_axis_plane(viewport.ray((v2i) Input::mouse_pos), Z, 0.0f);

    constexpr f32 raise_speed = 0.10f;
    auto drags = scene->registry.view<Dragging>();
    for (auto [entity, drag] : drags.each()) {
        v3 logic_offset = intersect - drag.start_intersect;
        drag.logic_position = drag.start_logic_position + logic_offset;
        
        drag.vertical_offset = 0.5f * math::smoothstep(drag.when, drag.when + raise_speed, scene->time);
    }
    
    auto _view = scene->registry.view<Dragging, ModelTransform>();
    for (auto [entity, dragging, transform] : _view.each()) {
        v3 pos = dragging.logic_position + v3(0.0f, 0.0f, dragging.vertical_offset);
        if (scene->registry.all_of<TransformLink>(entity)) {
            pos += scene->registry.get<TransformLink>(entity).offset;
        }
        transform.translation = pos;
    }

    auto posers = scene->registry.view<Dragging, PoseController>();
    for (auto [entity, _, poser] : posers.each()) {
        poser.target_state = "flail";
    }
}

void collision_update_system(Scene* scene) {
    ZoneScoped;

    if (scene->edit_mode)
        return;
    
    auto view = scene->registry.view<ModelTransform, Collision>();
    for (auto it1 = view.begin(); it1 != view.end(); it1++) {
        auto  entity1    = *it1;
        auto& transform1 = view.get<ModelTransform>(entity1);
        auto& collision1 = view.get<Collision>(entity1);

        auto begin2 = it1;
        begin2++;
        for (auto it2 = begin2; it2 != view.end(); it2++) {
            auto entity2 = *it2;
            auto& transform2 = view.get<ModelTransform>(entity2);
            auto& collision2 = view.get<Collision>(entity2);

            if (math::length(transform1.translation - transform2.translation) < (collision1.radius + collision2.radius)) {
                if (!collision1.with.contains(entity2))
                    collision1.with.insert(entity2);
                if (!collision2.with.contains(entity1))
                    collision2.with.insert(entity1);
            } else {
                if (collision1.with.contains(entity2))
                    collision1.with.erase(entity2);
                if (collision2.with.contains(entity1))
                    collision2.with.erase(entity1);
            }
        }
    }
}

void spawner_draw_system(Scene* scene) {
    for (auto [entity, spawner, m_transform, transform_link] : scene->registry.view<Spawner, ModelTransform, TransformLink>().each()) {
        transform_link.offset.z = 1.0f + 0.25f * math::sin(2.0f * scene->time);
        m_transform.scale = 0.6f;
        m_transform.rotation.yaw = 2.0f * (scene->time_scale == 0.0f ? Input::time : scene->time);
    }
}

}
