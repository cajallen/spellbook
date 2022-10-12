#include "systems.hpp"

#include <tracy/Tracy.hpp>

#include "lib_ext/fmt_geometry.hpp"

#include "astar.hpp"
#include "game.hpp"
#include "console.hpp"
#include "matrix_math.hpp"
#include "scene.hpp"
#include "components.hpp"

#include "renderer/render_scene.hpp"
#include "renderer/draw_functions.hpp"

namespace spellbook {

void travel_system(Scene* scene) {
    ZoneScoped;
    // construct grid to use for pathfinding
    auto slots     = scene->registry.view<GridSlot>();
    auto entities  = scene->registry.view<Traveler, Transform>();
    auto consumers = scene->registry.view<Consumer>();

    astar::Navigation nav;
    for (auto [entity, slot] : slots.each()) {
        if (slot.path)
            nav.positions.insert_back(slot.position.xy);
    }

    // handle actual traveling
    for (auto [entity, traveler, transform] : entities.each()) {
        bool has_path = !traveler.targets.empty();

        if (!has_path && !consumers.empty()) {
            int       random_consumer = math::random_s32() % s32(consumers.size());
            auto      consumer_entity = consumers[random_consumer];
            GridSlot* p_slot          = scene->registry.try_get<GridSlot>(consumer_entity);
            assert_else(p_slot);

            auto path = nav.find_path(math::round_cast(transform.translation.xy), p_slot->position.xy);
            for (auto it = path.begin(); it != path.end(); ++it) {
                traveler.targets.emplace_back(it->x, it->y, 0);
            }
        }
        assert_else(!math::is_nan(transform.translation.x)) {
            transform.translation = v3(0, 0, 0);
        }

        v3i  target_position = has_path ? traveler.targets.last() : math::round_cast(transform.translation);
        v3   velocity        = v3(target_position) - transform.translation;
        bool at_target       = math::length(velocity) < 0.01f;
        if (at_target && has_path) {
            traveler.targets.remove_back();
            velocity = v3(0);
        }
        f32 max_velocity = traveler.velocity * Input::delta_time;
        f32 min_velocity = 0.0f;
        if (!at_target)
            transform.translation += math::normalize(velocity) * math::clamp(math::length(velocity), min_velocity, max_velocity);
    }
}

// Sets the transform on Grid Slots
void grid_slot_transform_system(Scene* scene) {
    ZoneScoped;
    auto entities = scene->registry.view<Transform, GridSlot>();
    for (auto [entity, transform, grid_slot] : entities.each()) {
        transform.translation = (v3) grid_slot.position;
    }
}

// Creates the frame renderable for health bars
void health_draw_system(Scene* scene) {
    ZoneScoped;
    static int health_draw_system_i;
    auto       health_draw_system = scene->registry.view<Transform, Health, Model>();
    health_draw_system_i          = 0;
    for (auto [entity, transform, health, model] : health_draw_system.each()) {
        Name*  p_name = scene->registry.try_get<Name>(entity);
        string name   = p_name ? fmt_("{}:health", p_name->name) : fmt_("nameless_{}:health", health_draw_system_i++);

        string mesh_name = fmt_("cube_c{:.2f}_e{:.2f}", v3(0), v3(1));
        // TODO:
        // if (game.renderer.meshes.count(mesh_name) == 0) {
        //     game.renderer.upload_mesh(generate_cube(v3(0), v3(1)), false);
        // }
        //
        // if (game.renderer.materials.count("health_material") == 0) {
        //     MaterialCPU material_cpu = {.name = "health_material", .color_tint = palette::black, .emissive_tint = palette::green};
        //     game.renderer.upload_material(material_cpu, false);
        // }
        // if (game.renderer.materials.count("health_bar_material") == 0) {
        //     MaterialCPU material_cpu = {.name = "health_bar_material",
        //         .color_tint              = palette::black,
        //         .emissive_tint                = palette::white,
        //         .cull_mode                    = vuk::CullModeFlagBits::eFront};
        //     game.renderer.upload_material(material_cpu, false);
        // }

        if (health.value <= 0.0f) continue;

        float dir_to_camera = math::angle_to(scene->cameras.first().position.xy, transform.translation.xy);
        float thickness = 0.03f;

        m44 inner_matrix = math::translate(transform.translation + model.offset) * 
                           math::rotation(euler{dir_to_camera - math::PI / 2.0f, 0.0f}) *
                           math::translate(v3(-(1.0f - health.value) / 2.0f, 0.0f, 1.0f)) *
                           math::scale(v3(health.value * 0.5f, 0.1f, 0.1f));
        m44 outer_matrix = math::translate(v3(0.0f, 0.0f, 1.0) + transform.translation + model.offset) *
                           math::rotation(euler{dir_to_camera - math::PI / 2.0f, 0.0f}) * 
                           math::scale(v3(0.5f + thickness, 0.1f + thickness, 0.1f + thickness));

        // auto renderable1 = Renderable(name, mesh_name, "health_material", inner_matrix);
        // auto renderable2 = Renderable(name + "_bar", mesh_name, "health_bar_material", outer_matrix);
        // scene->render_scene.frame_renderables.insert_back(renderable1);
        // scene->render_scene.frame_renderables.insert_back(renderable2);
    }
}

// Uses the transform for models
void transform_system(Scene* scene) {
    ZoneScoped;
    for (auto [entity, model, transform] : scene->registry.view<Model, Transform>().each()) {
        // TODO: rewrite
    }
}
void spawner_system(Scene* scene) {
    ZoneScoped;
    for (auto [entity, spawner] : scene->registry.view<Spawner>().each()) {
        if (spawner.last_spawn + spawner.rate < Input::time) {
            spawner.last_spawn += spawner.rate;

            GridSlot* p_slot = scene->registry.try_get<GridSlot>(entity);
            assert_else(p_slot);

            static int i          = 0;
            auto       new_entity = scene->registry.create();
            scene->registry.emplace<Name>(new_entity, fmt_("fruit_{}", i++));
            scene->registry.emplace<Transform>(new_entity, (v3) p_slot->position);
            // TODO: scene->registry.emplace<Model>(new_entity, model, v3(0.5f));
            scene->registry.emplace<Health>(new_entity, 1.0f);
            scene->registry.emplace<Traveler>(new_entity);
            scene->registry.emplace<Collision>(new_entity, 0.3f);
        }
    }
}

void consumer_system(Scene* scene) {
    ZoneScoped;
    auto consumers = scene->registry.view<Consumer, Transform>();
    auto consumees = scene->registry.view<Traveler, Transform>();

    for (auto [e_consumer, consumer, consumer_transform] : consumers.each()) {
        for (auto [e_consumee, consumee, consumee_transform] : consumees.each()) {
            f32 dist = math::length(v2(consumer_transform.translation.xy) - consumee_transform.translation.xy);
            if (dist < consumer.consume_distance) {
                scene->registry.emplace<Killed>(e_consumee, Input::time);
                console({.str=fmt_("Om nom nom"), .group = "system.consumer"});
            }
        }
    }
}

void disposal_system(Scene* scene) {
    ZoneScoped;

    auto killed = scene->registry.view<Killed>();
    scene->registry.destroy(killed.begin(), killed.end());
}

void health_system(Scene* scene) {
    ZoneScoped;
    auto healths = scene->registry.view<Health>();
    for (auto [entity, health] : healths.each()) {
        if (health.value <= 0.001f) {
            scene->registry.emplace<Killed>(entity, Input::time);
        }
    }
}

void pyro_system(Scene* scene) {
    ZoneScoped;
    auto pyros = scene->registry.view<Pyro, Transform>();
    auto enemies = scene->registry.view<Health, Transform, Traveler>();

    for (auto [e_pyro, pyro, pyro_transform] : pyros.each()) {
        if (Input::time <= pyro.last_tick + pyro.rate)
            continue;
        
        pyro.last_tick += pyro.rate; // Don't skip the deltatime
        for (auto [e_enemy, enemy_health, enemy_transform, _] : enemies.each()) {
            if (e_pyro == e_enemy) continue;
            if (math::length(pyro_transform.translation - enemy_transform.translation) > pyro.radius) continue;
            
            enemy_health.value -= pyro.damage;
        }
    }
}

void selection_id_system(Scene* scene) {
    ZoneScoped;
    auto model_view = scene->registry.view<Model>();
    for (auto [entity, model] : model_view.each()) {
        // TODO: set selection_ids
    }
}

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
            auto transform = scene->registry.try_get<Transform>(scene->selected_entity);
            assert_else(transform);

            scene->registry.emplace<Dragging>(scene->selected_entity, Input::time, transform->translation, intersect);
        }
    }
}

void dragging_system(Scene* scene) {
    ZoneScoped;
    Viewport& viewport = scene->render_scene.viewport;
    v3 intersect = math::intersect_axis_plane(viewport.ray((v2i) Input::mouse_pos), Z, 0.0f);

    constexpr f32 raise_speed = 4.0f;

    auto _view = scene->registry.view<Dragging, Transform>();
    for (auto [entity, dragging, transform] : _view.each()) {
        v3 offset = intersect - dragging.start_intersect;
        v3 position = dragging.start_position + offset;

        transform.translation.z = math::min(transform.translation.z + raise_speed * Input::delta_time, 0.5f);
        transform.translation.xy = position.xy;
    }
}

void collision_update_system(Scene* scene) {
    ZoneScoped;

    auto view = scene->registry.view<Transform, Collision>();
    for (auto it1 = view.begin(); it1 != view.end(); it1++) {
        auto  entity1    = *it1;
        auto& transform1 = view.get<Transform>(entity1);
        auto& collision1 = view.get<Collision>(entity1);

        auto begin2 = it1;
        begin2++;
        for (auto it2 = begin2; it2 != view.end(); it2++) {
            auto entity2 = *it2;
            auto& transform2 = view.get<Transform>(entity2);
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

void roller_system(Scene* scene) {
    ZoneScoped;
    auto rollers = scene->registry.view<Roller, Transform>();

    static bool generated = false;
    if (!generated) {
        MaterialCPU material_cpu = {.name = "rollee_material", .file_name = "rollee_material", .color_tint = palette::slate_gray};
        game.renderer.upload_material(material_cpu, false);
        game.renderer.upload_mesh(generate_icosphere(3), false);
        generated = true;
    }

    for (auto [entity, roller, transform] : rollers.each()) {
        if (Input::time <= roller.last_tick + roller.rate)
            continue;
        roller.last_tick += roller.rate; // Don't skip the deltatime
        
        constexpr std::array<v2, 4> dirs { v2(-1, 0), v2( 0, 1), v2( 1, 0), v2( 0,-1) };

        for (int i = 0; i < 4; i++) {
            static int rollee_index = 0;
            auto       new_entity = scene->registry.create();

            // vector<slot<Renderable>> model;
            // model.insert_back(scene->render_scene.add_renderable(Renderable{fmt_("rollee_{}_renderable", rollee_index), "icosphere_3", "rollee_material", math::scale(roller.rollee_radius)}));

            scene->registry.emplace<Name>(new_entity, fmt_("rollee_{}", rollee_index));
            scene->registry.emplace<Transform>(new_entity, transform.translation + v3(0.2f * dirs[i], 0.0f), euler(), roller.rollee_radius);
            // TODO: scene->registry.emplace<Model>(new_entity, model, v3(0.5f));
            scene->registry.emplace<Rollee>(new_entity, entity, roller.rollee_speed * v3(dirs[i], 0.0f), roller.rollee_lifetime);
            scene->registry.emplace<Collision>(new_entity, roller.rollee_radius);
            rollee_index++;
        }
    }
}

void rollee_system(Scene* scene) {
    ZoneScoped;
    
    for (auto [entity, rollee] : scene->registry.view<Rollee>().each()) {
        rollee.lifetime -= Input::delta_time;

        if (rollee.lifetime <= 0.0f) {
            scene->registry.emplace<Killed>(entity, Input::time);
            continue;
        }

        if (!scene->registry.valid(rollee.roller)) {
            scene->registry.emplace<Killed>(entity, Input::time);
            continue;
        }

        auto p_transform = scene->registry.try_get<Transform>(entity);
        if (p_transform) {
            p_transform->translation += rollee.velocity * Input::delta_time;
        }

        auto p_collision = scene->registry.try_get<Collision>(entity);
        if (p_collision) {
            for (auto with : p_collision->with) {
                if (!scene->registry.valid(with))
                    continue;
                auto p_health = scene->registry.try_get<Health>(with);
                if (scene->registry.all_of<Traveler>(with)) {
                    p_health->value -= scene->registry.get<Roller>(rollee.roller).damage * Input::delta_time;
                }
            }
        }
        if (p_transform && p_collision) {
            p_transform->scale = math::mix(p_collision->radius,0.0f,math::smoothstep(0.2f, 0.0f, rollee.lifetime));
        }
    }
}

}
