#include "systems.hpp"

#include <tracy/Tracy.hpp>
#include <vuk/RenderGraph.hpp>
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>

#include "extension/fmt.hpp"
#include "extension/fmt_geometry.hpp"
#include "general/math/matrix_math.hpp"
#include "general/logger.hpp"
#include "renderer/render_scene.hpp"
#include "renderer/draw_functions.hpp"
#include "editor/console.hpp"
#include "editor/widget_system.hpp"
#include "game/game.hpp"
#include "game/input.hpp"
#include "game/scene.hpp"
#include "game/pose_controller.hpp"
#include "game/entities/components.hpp"
#include "game/entities/enemy.hpp"
#include "game/entities/spawner.hpp"
#include "game/entities/consumer.hpp"

namespace spellbook {

// Creates the frame renderable for health bars
void health_draw_system(Scene* scene) {
    ZoneScoped;

    if (scene->edit_mode)
        return;
    
    // dependencies
    static bool deps = false;
    static uint64 mesh_id;
    static uint64 health_friendly_id;
    static uint64 health_enemy_id;
    static uint64 health_buffer_id;
    static uint64 health_bar_id;
    if (!deps) {
        mesh_id = upload_mesh(generate_cube(v3(0), v3(1)));
        
        MaterialCPU health_friendly_material = {
            .file_path     = FilePath("health_friendly_material", true),
            .color_tint    = palette::black,
            .emissive_tint = palette::green
        };
        health_friendly_id = upload_material(health_friendly_material);
        MaterialCPU health_enemy_material = {
            .file_path     = FilePath("health_enemy_material", true),
            .color_tint    = palette::black,
            .emissive_tint = palette::fire_brick
        };
        health_enemy_id = upload_material(health_enemy_material);
        MaterialCPU material3_cpu = {
            .file_path     = FilePath("health_buffer_material", true),
            .color_tint    = palette::black,
            .emissive_tint = palette::yellow,
            .cull_mode = vuk::CullModeFlagBits::eFront
        };
        health_buffer_id = upload_material(material3_cpu);
        MaterialCPU material2_cpu = {
            .file_path     = FilePath("health_bar_material", true),
            .color_tint    = palette::black,
            .roughness_factor = 1.0f,
            .cull_mode     = vuk::CullModeFlagBits::eFront
        };
        health_bar_id = upload_material(material2_cpu);
        deps = true;
    }

    auto       health_draw_system = scene->registry.view<LogicTransform, Health>();
    for (auto [entity, logic_tfm, health] : health_draw_system.each()) {
        bool friendly = scene->registry.all_of<Lizard>(entity);
        if (health.value <= 0.0f)
            continue;

        float percentage = health.value / health.max_health->value();
        float percentage2 = health.buffer_value / health.max_health->value();
        auto link = scene->registry.try_get<TransformLink>(entity);
        v3 position = link ? logic_tfm.position + link->offset : logic_tfm.position + v3(0.5f, 0.5f, 0.0f);
        float dir_to_camera = math::angle_to(scene->camera.position.xy, position.xy);

        float width = friendly ? 0.7f : 0.5f;
        float vertical_offset = friendly ? 1.5f : 0.6f;
        float thickness = friendly ? 0.05f : 0.03f;
        
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

        auto renderable1 = Renderable{mesh_id, friendly ? health_friendly_id : health_enemy_id, (m44GPU) inner_matrix, {}, true};
        auto renderable2 = Renderable{mesh_id, health_bar_id, (m44GPU) outer_matrix, {}, true};
        auto renderable3 = Renderable{mesh_id, health_buffer_id, (m44GPU) buffer_matrix, {}, true};
        scene->render_scene.add_renderable(renderable1);
        scene->render_scene.add_renderable(renderable2);
        scene->render_scene.add_renderable(renderable3);
    }
}

// Uses the transform for models
void transform_system(Scene* scene) {
    auto& registry = scene->registry;
    ZoneScoped;
    // Transform Link
    {
        ZoneScopedN("Transform Link");
        for (auto [entity, link, logic_tfm, model_tfm] : registry.view<TransformLink, LogicTransform, ModelTransform>().each()) {
            // We disable links for dragging
            if (registry.any_of<Dragging>(entity))
                continue;

            Grounded* grounded = registry.try_get<Grounded>(entity);
            v3 step_up = grounded ? v3::Z * grounded->step_up : v3{};
            quat logic_quat = math::to_quat(math::normal_yaw(logic_tfm.normal, logic_tfm.yaw));
            model_tfm.set_rotation(math::normalize(math::slerp(model_tfm.rotation, logic_quat, 0.5f)));
            // model_tfm.set_rotation(logic_quat);
            model_tfm.set_translation(logic_tfm.position + link.offset + step_up);
        }
    }
    {
        ZoneScopedN("Apply Transform");
        // Apply transforms to renderables
        for (auto [entity, model, transform] : registry.view<Model, ModelTransform>().each()) {
            // Note, this isn't correct, if the transform is accessed early the dirty flag will clear,
            // but it's just for investigating optimization options
            if (!transform.renderable_dirty)
                continue;
            for (auto& [node, renderable] : model.model_gpu.renderables) {
                ZoneScoped;
                renderable->transform = (m44GPU) (transform.get_transform() * node->cached_transform);
            }
            transform.renderable_dirty = false;
        }
    }
}

void disposal_system(Scene* scene) {
    ZoneScoped;
    for (auto [entity, transform, drop_chance, killed] : scene->registry.view<LogicTransform, DropChance, Killed>().each()) {
        if (killed.drop) {
            for (auto& entry : drop_chance.entries) {
                if (math::random_float(1.0f) < entry.drop_chance) {
                    float phase = math::random_float(math::TAU);
                    float radius = math::random_float(0.2f);
                    v3 offset = radius * v3(math::cos(phase), math::sin(phase), 0.0f);
                    instance_prefab(scene, load_asset<BeadPrefab>(entry.bead_prefab_path), transform.position + offset);
                }
            }
        }
    }
    auto killed = scene->registry.view<Killed>();
    scene->registry.destroy(killed.begin(), killed.end());

    if (scene->edit_mode)
        return;
}

void health_system(Scene* scene) {
    ZoneScoped;
    auto healths = scene->registry.view<Health>();
    for (auto [entity, health] : healths.each()) {
        for (auto& [dotter, dot] : health.dots) {
            damage(scene, dotter, entity, dot->value() * scene->delta_time, v3(0.0f));
        }
        float dh = 6.0f * (health.buffer_value - health.value) + 0.3f;
        health.buffer_value = math::max(health.buffer_value - dh * scene->delta_time, health.value);

        if (scene->registry.any_of<Killed>(entity))
            continue;
        
        if (health.value <= 0.001f) {
            scene->registry.emplace<Killed>(entity, true, scene->time);
        }

        if (health.regen)
            health.value += health.regen->value() * scene->delta_time;
        health.value = math::min(health.value, health.max_health->value());
    }
}

void selection_id_system(Scene* scene) {
    ZoneScoped;
    auto model_view = scene->registry.view<Model>();
    for (auto [entity, model] : model_view.each()) {
        uint32 id = (uint32) entity;
        for (auto [_, r] : model.model_gpu.renderables) {
            r->selection_id = id;
        }
    }
}



// Adds dragging to entities
void dragging_update_system(Scene* scene) {
    ZoneScoped;
    if (scene->render_scene.fut_query_result.get_control()) {
        auto result_buf = *scene->render_scene.fut_query_result.get<vuk::Buffer>(*game.renderer.global_allocator, game.renderer.compiler);
        uint32 result_int = *((uint32*) result_buf.mapped_ptr);
        scene->render_scene.fut_query_result = {};

        if (scene->registry.valid((entt::entity) result_int)) {
            scene->select_entity((entt::entity) result_int);
        }
    }
}

void dragging_system(Scene* scene) {
    ZoneScoped;
    Viewport& viewport = scene->render_scene.viewport;
    // v3 intersect = math::intersect_axis_plane(viewport.ray((v2i) Input::mouse_pos), Z, 0.0f);
    
    constexpr float raise_speed = 0.10f;
    auto drags = scene->registry.view<LogicTransform, Dragging, Draggable>();
    
    v3i cell;
    bool has_new_cell = false;
    if (drags.size_hint() > 0) {
        has_new_cell = scene->get_object_placement(cell);
    }
    
    for (auto [entity, transform, drag, draggable] : drags.each()) {
        if (has_new_cell) {
            drag.target_position = v3(cell);
            drag.potential_logic_position = v3(cell);
        }
    }
    
    auto _view = scene->registry.view<Dragging, ModelTransform>();
    for (auto [entity, drag, transform] : _view.each()) {
        float vertical_offset = drag.drag_height * math::smoothstep(drag.start_time, drag.start_time + raise_speed, scene->time);
        v3 pos = drag.target_position + v3(0.0f, 0.0f, vertical_offset);
        if (scene->registry.all_of<TransformLink>(entity)) {
            pos += scene->registry.get<TransformLink>(entity).offset;
        }
        transform.set_translation(pos);
    }

    for (auto [entity, lizard, drag] : scene->registry.view<Lizard, Dragging>().each()) {
        draw_lizard_dragging_preview(scene, entity);
    }
    
    auto posers = scene->registry.view<Dragging, PoseController>();
    for (auto [entity, _, poser] : posers.each()) {
        poser.set_state(AnimationState_Flail);
    }
}

void collision_update_system(Scene* scene) {
    ZoneScoped;

    if (scene->edit_mode)
        return;
    
    auto view = scene->registry.view<LogicTransform, Collision>();
    for (auto it1 = view.begin(); it1 != view.end(); it1++) {
        auto  entity1    = *it1;
        auto& transform1 = view.get<LogicTransform>(entity1);
        auto& collision1 = view.get<Collision>(entity1);

        auto begin2 = it1;
        begin2++;
        for (auto it2 = begin2; it2 != view.end(); it2++) {
            auto entity2 = *it2;
            auto& transform2 = view.get<LogicTransform>(entity2);
            auto& collision2 = view.get<Collision>(entity2);

            if (math::length(transform1.position - transform2.position) < (collision1.radius + collision2.radius)) {
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

void emitter_system(Scene* scene) {
    for (auto [entity, emitter_comp, logic_transform] : scene->registry.view<EmitterComponent, LogicTransform>().each()) {
        for (auto& [id, emitter_gpu] : emitter_comp.emitters) {
            if (emitter_gpu->emitter_cpu.position != logic_transform.position) {
                emitter_gpu->emitter_cpu.position = logic_transform.position;
                emitter_gpu->update_from_cpu(emitter_gpu->emitter_cpu);
            }
        }
    }
}

void pickup_system(Scene* scene) {
    auto lizard_view = scene->registry.view<Lizard, LogicTransform>();
    for (auto [entity, pickup, logic_tfm] : scene->registry.view<Pickup, LogicTransform>().each()) {
        v3 closest_position = logic_tfm.position;
        float closest_distance = FLT_MAX;
        for (auto [lizard_entity, lizard, lizard_transform] : lizard_view.each()) {
            float distance = math::length(logic_tfm.position - lizard_transform.position);
            if (distance < 0.2f) {
                scene->player.bank.beads[pickup.bead_type]++;
                scene->registry.emplace<Killed>(entity);
            } else {
                if (math::abs(logic_tfm.position.z - lizard_transform.position.z) >= 0.8f)
                    continue;

                if (distance < closest_distance) {
                    closest_position = lizard_transform.position;
                    closest_distance = distance;
                }
            }
        }

        if (closest_distance < 10.0f) {
            v3 to_position = closest_position - logic_tfm.position;
            to_position = math::normalize(to_position) * scene->delta_time * 2.0f;
            logic_tfm.position += to_position;
        }

        
        pickup.cycle_point = math::mod(pickup.cycle_point + 0.4f * scene->delta_time, 10.0f);
        
        logic_tfm.yaw = pickup.cycle_point * math::TAU;
        scene->registry.get<ModelTransform>(entity).set_rotation(math::to_quat(euler{
            .yaw = pickup.cycle_point * math::TAU,
            .pitch = math::sin(pickup.cycle_point * math::TAU) * 0.05f
        }));
        scene->registry.get<TransformLink>(entity).offset.z = 0.1f * math::sin(pickup.cycle_point * math::TAU) + 0.3f;
    }
}

float calculate_step_up(Scene* scene, entt::entity id, v3 pos) {
    v2 closest_shrine_dist = v2(FLT_MAX, FLT_MAX);
    v2 closest_platform_dist = v2(FLT_MAX, FLT_MAX);
    for (auto [entity2, shrine_logic_tfm, shrine] : scene->registry.view<LogicTransform, Shrine>().each()) {
        if (id == entity2)
            continue;
        if (math::abs(shrine_logic_tfm.position.z -pos.z) > 0.1f)
            continue;

        v2 potential_new_dist = math::abs(shrine_logic_tfm.position.xy -pos.xy);
        if (math::length(potential_new_dist) < math::length(closest_shrine_dist))
            closest_shrine_dist = potential_new_dist;
    }
    for (auto [entity2, platform_logic_tfm, platform] : scene->registry.view<LogicTransform, CastingPlatform>().each()) {
        if (id == entity2)
            continue;
        if (math::abs(platform_logic_tfm.position.z -pos.z) > 0.1f)
            continue;

        v2 potential_new_dist = math::abs(platform_logic_tfm.position.xy -pos.xy);
        if (math::length(potential_new_dist) < math::length(closest_platform_dist))
            closest_platform_dist = potential_new_dist;
    }
    for (auto [entity2, platform_logic_tfm, platform] : scene->registry.view<LogicTransform, Spawner>().each()) {
        if (id == entity2)
            continue;
        if (math::abs(platform_logic_tfm.position.z -pos.z) > 0.1f)
            continue;

        v2 potential_new_dist = math::abs(platform_logic_tfm.position.xy -pos.xy);
        if (math::length(potential_new_dist) < math::length(closest_platform_dist))
            closest_platform_dist = potential_new_dist;
    }

        
    if (math::length(closest_platform_dist) < 0.5f) {
        return math::map_range(math::length(closest_platform_dist), {0.38f, 0.5f}, {0.1f, 0.0f});
    }
    if (math::length(closest_shrine_dist) < 0.65f) {
        return math::map_range(math::length(closest_shrine_dist), {0.4f, 0.65f}, {0.2f, 0.0f});
    }
    return 0.0f;
}

void scene_vertical_offset_system(Scene* scene) {
    ZoneScoped;
    for (auto [entity, logic_tfm, grounded] : scene->registry.view<LogicTransform, Grounded>().each()) {
        grounded.step_up = calculate_step_up(scene, entity, logic_tfm.position);
    }
}

}
