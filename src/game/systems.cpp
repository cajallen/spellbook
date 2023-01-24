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
    static string health_buffer_name;
    static string health_bar_name;
    if (!deps) {
        mesh_name = upload_mesh(generate_cube(v3(0), v3(1)));
        
        MaterialCPU material1_cpu = {
            .file_path     = "health_material",
            .color_tint    = palette::black,
            .emissive_tint = palette::green
        };
        health_name = upload_material(material1_cpu);
        MaterialCPU material3_cpu = {
            .file_path     = "health_buffer_material",
            .color_tint    = palette::black,
            .emissive_tint = palette::yellow
        };
        health_buffer_name = upload_material(material3_cpu);
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
        float percentage2 = health.buffer_value / health.max_health.value();
        auto link = scene->registry.try_get<TransformLink>(entity);
        v3 position = link ? transform.position + link->offset : transform.position;

        float dir_to_camera = math::angle_to(scene->camera.position.xy, position.xy);
        
        constexpr float thickness = 0.03f;
        constexpr float width = 0.6f;
        m44 inner_matrix = math::translate(position) * 
                           math::rotation(euler{dir_to_camera - math::PI / 2.0f, 0.0f}) *
                           math::translate(v3(-(1.0f - percentage) * 0.5f * width, 0.0f, 0.5f)) *
                           math::scale(v3(percentage * 0.5f * width, thickness, thickness));
        m44 buffer_matrix = math::translate(position) * 
                           math::rotation(euler{dir_to_camera - math::PI / 2.0f, 0.0f}) *
                           math::translate(v3(-(1.0f - percentage2) * 0.5f * width, 0.0f, 0.5f)) *
                           math::scale(v3(percentage2 * 0.5f * width, thickness, thickness) * 0.999f);
        m44 outer_matrix = math::translate(v3(0.0f, 0.0f, 0.5) + position) *
                           math::rotation(euler{dir_to_camera - math::PI * 0.5f, 0.0f}) * 
                           math::scale(v3(0.5f * width, thickness, thickness));

        auto renderable1 = Renderable{mesh_name, health_name, (m44GPU) inner_matrix, {}, true};
        auto renderable2 = Renderable{mesh_name, health_bar_name, (m44GPU) outer_matrix, {}, true};
        auto renderable3 = Renderable{mesh_name, health_buffer_name, (m44GPU) buffer_matrix, {}, true};
        scene->render_scene.add_renderable(renderable1);
        scene->render_scene.add_renderable(renderable2);
        scene->render_scene.add_renderable(renderable3);
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
        
        m_transform.set_translation(l_transform.position + link.offset);
    }
    // Apply transforms to renderables
    for (auto [entity, model, transform] : registry.view<Model, ModelTransform>().each()) {
        for (auto& [node, renderable] : model.model_gpu.renderables) {
            renderable->transform = (m44GPU) (transform.get_transform() * node->cached_transform);
        }
    }
}

void consumer_system(Scene* scene) {
    ZoneScoped;
    auto consumers = scene->registry.view<Consumer, LogicTransform>();
    auto consumees = scene->registry.view<Traveler, LogicTransform>();

    for (auto [e_consumer, consumer, consumer_transform] : consumers.each()) {
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
        float dh = 2.0f * (health.buffer_value - health.value) + 0.2f;
        health.buffer_value = math::max(health.buffer_value - dh * scene->delta_time, health.value);

        auto pos = scene->registry.get<LogicTransform>(entity);
        
        if (health.hurt_emitter) {
            health.hurt_emitter->emitting = health.hurt_until > Input::time;
            if (health.hurt_emitter->emitting) {
                float length = math::length(health.hurt_emitter->emitter_cpu.velocity);
                health.hurt_emitter->emitter_cpu.velocity = length * health.hurt_direction;
                health.hurt_emitter->emitter_cpu.position = pos.position + v3(0.5f);
                health.hurt_emitter->update_from_cpu(health.hurt_emitter->emitter_cpu);
            }
        }
        
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
            if (poser.reset_time) {
                console({.str="Reset"});
                skeleton_cpu.time = 0.0f;
                poser.reset_time = false;
            }
            skeleton_cpu.time += Input::delta_time;
            auto& poses = skeleton_cpu.poses;
            if (poser.target_state == "flail") {
                if (poses.contains("flail_1") && poses.contains("flail_2")) {
                    if (math::mod(skeleton_cpu.time, poser.time_to_target) < poser.time_to_target * 0.5f) {
                        if (skeleton_cpu.current_pose != "flail_1") {
                            skeleton_cpu.load_pose("flail_1", true, poser.time_to_target * 0.5f);
                            poser.time_to_target = poser.cycle_duration;
                        }
                    } else {
                        if (skeleton_cpu.current_pose != "flail_2") {
                            skeleton_cpu.load_pose("flail_2", true, poser.time_to_target * 0.5f);
                            poser.time_to_target = poser.cycle_duration;
                        }
                    }
                }
                else {
                    log_warning("Pose doesn't support flail");
                }
            }
            if (poser.target_state == "attacking") {
                if (poses.contains("windup") || poses.contains("followthrough")) {
                    if (skeleton_cpu.time < poser.time_to_target * 0.5f) {
                        if (skeleton_cpu.current_pose != "windup")
                            skeleton_cpu.load_pose("windup", true, poser.time_to_target * 0.5f);
                    } else {
                        if (skeleton_cpu.current_pose != "followthrough") {
                            skeleton_cpu.load_pose("followthrough", true, poser.time_to_target * 0.5f);
                        }
                    }
                }
                else {
                    log_warning("Pose doesn't support attacking");
                }
            }
            if (poser.target_state == "default") {
                if (poses.contains("idle_1") && poses.contains("idle_2")) {
                    if (math::mod(skeleton_cpu.time, poser.time_to_target) < poser.time_to_target * 0.5f) {
                        if (skeleton_cpu.current_pose != "idle_1") {
                            skeleton_cpu.load_pose("idle_1", true, poser.time_to_target * 0.5f);
                            poser.time_to_target = poser.cycle_duration;
                        }
                    } else {
                        if (skeleton_cpu.current_pose != "idle_2") {
                            skeleton_cpu.load_pose("idle_2", true, poser.time_to_target * 0.5f);
                            poser.time_to_target = poser.cycle_duration;
                        }
                    }
                }
                else if (poses.contains("default")) {
                    if (skeleton_cpu.current_pose != "default")
                        skeleton_cpu.load_pose("default", poser.time_to_target != 0.0f, poser.time_to_target);
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
            scene->select_entity((entt::entity) result_int);
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
        drag.target_position = drag.start_logic_position + logic_offset;

        entt::entity potential_entity = scene->get_tile(math::round_cast(drag.target_position.xy));
        auto potential_transform = scene->registry.try_get<LogicTransform>(potential_entity);
        if (potential_transform) {
            drag.potential_logic_position = potential_transform->position + v3(0.0f, 0.0f, 1.0f);
        }
        drag.target_position.z = drag.potential_logic_position.z;
    }
    
    auto _view = scene->registry.view<Dragging, ModelTransform>();
    for (auto [entity, drag, transform] : _view.each()) {
        f32 vertical_offset = 0.5f * math::smoothstep(drag.start_time, drag.start_time + raise_speed, scene->time);
        v3 pos = drag.target_position + v3(0.0f, 0.0f, vertical_offset);
        if (scene->registry.all_of<TransformLink>(entity)) {
            pos += scene->registry.get<TransformLink>(entity).offset;
        }
        transform.set_translation(pos);
    }

    auto posers = scene->registry.view<Dragging, PoseController>();
    for (auto [entity, _, poser] : posers.each()) {
        poser.set_state("flail", 0.2f, 0.75f);
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

void lizard_targeting_system(Scene* scene) {
    for (auto [entity, lizard] : scene->registry.view<Lizard>().each()) {
        if (!lizard.basic_ability->post_trigger_timer->ticking && lizard.basic_ability->targeting_callback)
            lizard.basic_ability->targeting_callback(lizard.basic_ability->targeting_payload);
    }
}

void lizard_casting_system(Scene* scene) {
    for (auto [entity, lizard] : scene->registry.view<Lizard>().each()) {
        if (lizard.basic_ability->has_target && lizard.basic_ability->ready_to_cast())
            lizard.basic_ability->request_cast();
    }
}

void spawner_draw_system(Scene* scene) {
    for (auto [entity, spawner, m_transform, transform_link] : scene->registry.view<Spawner, ModelTransform, TransformLink>().each()) {
        transform_link.offset.z = 1.0f + 0.25f * math::sin(2.0f * scene->time);
        m_transform.set_scale(v3(0.6f));
        m_transform.set_rotation(euler{
            .yaw = 2.0f * (scene->time_scale == 0.0f ? Input::time : scene->time),
            .pitch = m_transform.rotation.pitch,
            .roll = m_transform.rotation.roll
        });
    }
}

void emitter_system(Scene* scene) {
    for (auto [entity, emitter_comp, logic_transform] : scene->registry.view<EmitterComponent, LogicTransform>().each()) {
        if (emitter_comp.emitter->emitter_cpu.position != logic_transform.position) {
            emitter_comp.emitter->emitter_cpu.position = logic_transform.position;
            emitter_comp.emitter->update_from_cpu(emitter_comp.emitter->emitter_cpu);
        }
    }
}

void visual_tile_widget_system(Scene* scene) {
    static string mesh_name;
    static string mat_off_name;
    static string mat_on_name;
    if (mesh_name.empty()) {
        mesh_name = upload_mesh(generate_icosphere(3), false);
        mat_off_name = upload_material({.file_path = "mat_off_name", .color_tint = palette::gray_1}, false);
        mat_on_name = upload_material({.file_path = "mat_on_name", .color_tint = palette::gray_1, .emissive_tint = palette::spellbook_1}, false);
    }
    for (auto [entity, vtsw] : scene->registry.view<VisualTileSetWidget>().each()) {
        if (vtsw.tile_set != nullptr) {
            auto& tiles = vtsw.tile_set->tiles;
            u32 width = u32(math::ceil(math::sqrt(f32(tiles.size()))));
            u32 i = 0;
            for (VisualTilePrefab& tile_entry : tiles) {
                v3 pos = (v3(i % width, i / width, 0.0f) - v3(0.5f * width, 0.5f * width, 0.0f)) * 3.0f;
                i++;
                for (int c = 0; c < 8; c++) {
                    auto& r = scene->render_scene.quick_mesh(mesh_name, tile_entry.corners[c] > 0 ? mat_on_name : mat_off_name, true);           
                    r.transform = m44GPU(math::translate(pos + v3(visual_direction_offsets[c]) - v3(0.5f)) * math::scale(0.05f));
                }
            }
            
        }
    }
}

}
