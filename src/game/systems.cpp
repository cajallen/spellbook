#include "systems.hpp"

#include <tracy/Tracy.hpp>
#include <entt/entt.hpp>
#include <vuk/RenderGraph.hpp>

#include "extension/fmt.hpp"
#include "extension/fmt_geometry.hpp"
#include "general/astar.hpp"
#include "general/matrix_math.hpp"
#include "general/logger.hpp"
#include "game/game.hpp"
#include "game/scene.hpp"
#include "game/components.hpp"
#include "game/input.hpp"
#include "game/pose_controller.hpp"
#include "editor/console.hpp"
#include "editor/widget_system.hpp"
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
        if (slot.ramp) {
            nav.ramps[v3i(logic_pos.position)] = slot.direction;
            continue;
        }
        if (slot.path)
            nav.solids.set(v3i(logic_pos.position));
    }
    
    // handle actual traveling
    for (auto [entity, traveler, transform] : entities.each()) {
        bool has_path = !traveler.targets.empty();

        if (!has_path && !consumers.empty()) {
            int       random_consumer = math::random_s32() % s32(consumers.size());
            auto      consumer_entity = consumers[random_consumer];
            LogicTransform* consumer_position          = scene->registry.try_get<LogicTransform>(consumer_entity);
            assert_else(consumer_position);

            auto path = nav.find_path(math::round_cast(transform.position), v3i(consumer_position->position));
            for (auto it = path.begin(); it != path.end(); ++it) {
                traveler.targets.push_back(*it);
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
    static string health_friendly_name;
    static string health_enemy_name;
    static string health_buffer_name;
    static string health_bar_name;
    if (!deps) {
        mesh_name = upload_mesh(generate_cube(v3(0), v3(1)));
        
        MaterialCPU health_friendly_material = {
            .file_path     = "health_friendly_material",
            .color_tint    = palette::black,
            .emissive_tint = palette::green
        };
        health_friendly_name = upload_material(health_friendly_material);
        MaterialCPU health_enemy_material = {
            .file_path     = "health_enemy_material",
            .color_tint    = palette::black,
            .emissive_tint = palette::fire_brick
        };
        health_enemy_name = upload_material(health_enemy_material);
        MaterialCPU material3_cpu = {
            .file_path     = "health_buffer_material",
            .color_tint    = palette::black,
            .emissive_tint = palette::yellow,
            .cull_mode = vuk::CullModeFlagBits::eFront
        };
        health_buffer_name = upload_material(material3_cpu);
        MaterialCPU material2_cpu = {
            .file_path     = "health_bar_material",
            .color_tint    = palette::black,
            .roughness_factor = 1.0f,
            .cull_mode     = vuk::CullModeFlagBits::eFront
        };
        health_bar_name = upload_material(material2_cpu);
        deps = true;
    }

    auto       health_draw_system = scene->registry.view<LogicTransform, Health>();
    for (auto [entity, transform, health] : health_draw_system.each()) {
        bool friendly = scene->registry.all_of<Lizard>(entity);
        if (health.value <= 0.0f)
            continue;

        float percentage = health.value / health.max_health.value();
        float percentage2 = health.buffer_value / health.max_health.value();
        auto link = scene->registry.try_get<TransformLink>(entity);
        v3 position = link ? transform.position + link->offset : transform.position;
        float dir_to_camera = math::angle_to(scene->camera.position.xy, position.xy);

        float width = friendly ? 0.7f : 0.6f;
        float vertical_offset = friendly ? 1.2f : 0.5f;
        float thickness = friendly ? 0.05f : 0.035f;
        
        m44 inner_matrix = math::translate(position) * 
                           math::rotation(euler{dir_to_camera - math::PI / 2.0f, 0.0f}) *
                           math::translate(v3(-(1.0f - percentage) * 0.5f * width, 0.0f, vertical_offset)) *
                           math::scale(v3(percentage * 0.5f * width, thickness, thickness));
        m44 buffer_matrix = math::translate(position) * 
                           math::rotation(euler{dir_to_camera - math::PI / 2.0f, 0.0f}) *
                           math::translate(v3(-(1.0f - percentage2) * 0.5f * width, 0.0f, vertical_offset)) *
                           math::scale(v3(percentage2 * 0.5f * width, thickness, thickness) * 0.9f);
        m44 outer_matrix = math::translate(v3(0.0f, 0.0f, vertical_offset) + position) *
                           math::rotation(euler{dir_to_camera - math::PI * 0.5f, 0.0f}) * 
                           math::scale(v3(0.5f * width, thickness, thickness));

        auto renderable1 = Renderable{mesh_name, friendly ? health_friendly_name : health_enemy_name, (m44GPU) inner_matrix, {}, true};
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
    
    for (auto [entity, transform, drop_chance, killed] : scene->registry.view<LogicTransform, DropChance, Killed>().each()) {
        if (math::random_f32(1.0f) < drop_chance.drop_chance) {
            instance_prefab(scene, load_asset<BeadPrefab>(drop_chance.bead_prefab_path), transform.position);
        }
    }
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
    auto drags = scene->registry.view<LogicTransform, Dragging, Draggable>();
    for (auto [entity, transform, drag, draggable] : drags.each()) {
        v3 logic_offset = intersect - drag.start_intersect;
        drag.target_position = drag.start_logic_position + logic_offset;

        entt::entity potential_entity = scene->get_tile(math::round_cast(drag.target_position.xy));
        auto potential_transform = scene->registry.try_get<LogicTransform>(potential_entity);
        if (potential_transform) {
            v3 new_potential_pos = potential_transform->position + v3(0.0f, 0.0f, 1.0f);
            if (math::max(math::abs(new_potential_pos.x - transform.position.x), math::abs(new_potential_pos.x - transform.position.x)) <= draggable.drag_distance)
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
        poser.set_state(PoseController::State_Flailing, 0.2f);
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

void pickup_system(Scene* scene) {
    auto lizard_view = scene->registry.view<Lizard, LogicTransform>();
    for (auto [entity, pickup, bead_transform] : scene->registry.view<Pickup, LogicTransform>().each()) {
        v3 closest_position = bead_transform.position;
        float closest_distance = FLT_MAX;
        for (auto [lizard_entity, lizard, lizard_transform] : lizard_view.each()) {
            float distance = math::length(bead_transform.position - lizard_transform.position);
            if (distance < 0.2f) {
                scene->player.bank.beads[pickup.bead_type]++;
                scene->registry.emplace<Killed>(entity);
            } else {
                if (math::abs(bead_transform.position.z - lizard_transform.position.z) >= 0.8f)
                    continue;

                if (distance < closest_distance) {
                    closest_position = lizard_transform.position;
                    closest_distance = distance;
                }
            }
        }

        if (closest_distance < 10.0f) {
            v3 to_position = closest_position - bead_transform.position;
            to_position = math::normalize(to_position) * scene->delta_time * 2.0f;
            bead_transform.position += to_position;
        }

        
        pickup.cycle_point = math::mod(pickup.cycle_point + 0.4f * scene->delta_time, 10.0f);
        
        bead_transform.rotation.yaw = pickup.cycle_point * math::TAU;
        bead_transform.rotation.pitch = math::sin(pickup.cycle_point * math::TAU) * 0.05f;
        scene->registry.get<ModelTransform>(entity).rotation.yaw = pickup.cycle_point * math::TAU;
        scene->registry.get<ModelTransform>(entity).rotation.pitch = math::sin(pickup.cycle_point * math::TAU) * 0.05f;
        scene->registry.get<TransformLink>(entity).offset.z = 0.1f * math::sin(pickup.cycle_point * math::TAU) + 0.3f;
    }
}

}
