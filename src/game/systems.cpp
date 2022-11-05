#include "systems.hpp"

#include <tracy/Tracy.hpp>
#include <entt/entt.hpp>

#include "lib_ext/fmt_geometry.hpp"

#include "astar.hpp"
#include "game.hpp"
#include "console.hpp"
#include "matrix_math.hpp"
#include "scene.hpp"
#include "components.hpp"
#include "hash.hpp"
#include "input.hpp"

#include "renderer/render_scene.hpp"
#include "renderer/draw_functions.hpp"

#include "scene.hpp"

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
            nav.positions.insert_back(v2i(logic_pos.position.xy));
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

        v3i  target_position = has_path ? traveler.targets.last() : math::round_cast(transform.position);
        v3   velocity        = v3(target_position) - transform.position;
        bool at_target       = math::length(velocity) < 0.01f;
        if (at_target && has_path) {
            traveler.targets.remove_back();
            velocity = v3(0);
        }
        f32 max_velocity = traveler.max_speed.value() * Input::delta_time;
        f32 min_velocity = 0.0f;
        if (!at_target)
            transform.position += math::normalize(velocity) * math::clamp(math::length(velocity), min_velocity, max_velocity);
    }
}

// Creates the frame renderable for health bars
void health_draw_system(Scene* scene) {
    ZoneScoped;

    // dependencies
    string mesh_name = fmt_("cube_c{:.2f}_e{:.2f}", v3(0), v3(1));
    u64 cube_hash = hash_data(mesh_name.data(), mesh_name.size());
    if (!game.renderer.mesh_cache.contains(cube_hash)) {
        game.renderer.upload_mesh(generate_cube(v3(0), v3(1)));
    }

    string health_name = "health_material";
    u64 health_hash = hash_data(health_name.data(), health_name.size());
    if (!game.renderer.mesh_cache.contains(health_hash)) {
        MaterialCPU material_cpu = {
            .file_path     = health_name,
            .color_tint    = palette::black,
            .emissive_tint = palette::green
        };
        game.renderer.upload_material(material_cpu);
    }

    string health_bar_name = "health_bar_material";
    u64 health_bar_hash = hash_data(health_bar_name.data(), health_bar_name.size());
    if (!game.renderer.mesh_cache.contains(health_bar_hash)) {
        MaterialCPU material_cpu = {
            .file_path     = health_bar_name,
            .color_tint    = palette::black,
            .emissive_tint = palette::white,
            .cull_mode     = vuk::CullModeFlagBits::eFront
        };
        game.renderer.upload_material(material_cpu);
    }

    auto       health_draw_system = scene->registry.view<ModelTransform, Health, Model>();
    for (auto [entity, transform, health, model] : health_draw_system.each()) {
        if (health.value <= 0.0f)
            continue;

        float dir_to_camera = math::angle_to(scene->cameras.first().position.xy, transform.translation.xy);
        float thickness = 0.03f;

        m44 inner_matrix = math::translate(transform.translation) * 
                           math::rotation(euler{dir_to_camera - math::PI / 2.0f, 0.0f}) *
                           math::translate(v3(-(1.0f - health.value) / 2.0f, 0.0f, 1.0f)) *
                           math::scale(v3(health.value * 0.5f, 0.1f, 0.1f));
        m44 outer_matrix = math::translate(v3(0.0f, 0.0f, 1.0) + transform.translation) *
                           math::rotation(euler{dir_to_camera - math::PI / 2.0f, 0.0f}) * 
                           math::scale(v3(0.5f + thickness, 0.1f + thickness, 0.1f + thickness));

        auto renderable1 = Renderable{mesh_name, health_name, inner_matrix, true};
        auto renderable2 = Renderable{mesh_name, health_bar_name, outer_matrix, true};
        scene->render_scene.renderables.emplace(renderable1);
        scene->render_scene.renderables.emplace(renderable2);
    }
}

// Uses the transform for models
void transform_system(Scene* scene) {
    ZoneScoped;
    for (auto [entity, attach, transform] : scene->registry.view<LogicTransformAttach, LogicTransform>().each()) {
        if (scene->registry.valid(attach.to)) {
            transform = scene->registry.get<LogicTransform>(attach.to);
        } else {
            console_error("Invalid attachment", "components.transform", ErrorType_Warning);
            scene->registry.erase<LogicTransformAttach>(entity);
        }
    }
    for (auto [entity, model, transform] : scene->registry.view<Model, ModelTransform>().each()) {
        auto link = scene->registry.try_get<TransformLink>(entity);
        if (link != nullptr) {
            transform.translation = scene->registry.get<LogicTransform>(entity).position + link->offset;
        }
        m44 transform_matrix = math::translate(transform.translation) * math::rotation(transform.rotation) * math::scale(transform.scale);
        int i = 0;
        for (auto renderable : model.model_gpu.renderables) {
            renderable->transform = transform_matrix * model.model_cpu.nodes[i++]->calculate_transform();
        }
    }
}

void tower_system(Scene* scene) {
    ZoneScoped;
    for (auto [entity, tower, transform] : scene->registry.view<Tower, ModelTransform>().each()) {
        tower.current_rotation += tower.rotation_speed * Input::delta_time;
        transform.rotation.yaw = tower.current_rotation;

        auto& clouds_transform = scene->registry.get<ModelTransform>(tower.clouds);
        clouds_transform.rotation.yaw = 0.75 * tower.current_rotation;
        clouds_transform.scale = math::abs(math::sin(Input::time));
    }

    pyro_system(scene);
    roller_system(scene);
    rollee_system(scene);
}


void consumer_system(Scene* scene) {
    ZoneScoped;
    auto consumers = scene->registry.view<Consumer, LogicTransform>();
    auto consumees = scene->registry.view<Traveler, LogicTransform>();

    for (auto [e_consumer, consumer, consumer_transform] : consumers.each()) {
        auto consumer_mtransform = scene->registry.try_get<ModelTransform>(e_consumer);
        if (consumer_mtransform) {
            consumer_mtransform->rotation.yaw += Input::delta_time * math::TAU / 6.f;
            consumer_mtransform->rotation.pitch += Input::delta_time * math::TAU / 11.f;
            consumer_mtransform->rotation.roll += Input::delta_time * math::TAU / 19.f;
        }
        for (auto [e_consumee, consumee, consumee_transform] : consumees.each()) {
            if (scene->registry.any_of<Killed>(e_consumee))
                continue;
            f32 dist = math::length(v2(consumer_transform.position.xy) - consumee_transform.position.xy);
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
    auto pyros = scene->registry.view<Pyro, ModelTransform>();
    auto enemies = scene->registry.view<Health, ModelTransform, Traveler>();

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
        for (auto p_renderable : model.model_gpu.renderables) {
            p_renderable->selection_id = (u32) entity;
        }
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
            auto transform = scene->registry.try_get<ModelTransform>(scene->selected_entity);
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

    auto _view = scene->registry.view<Dragging, LogicTransform>();
    for (auto [entity, dragging, transform] : _view.each()) {
        v3 offset = intersect - dragging.start_intersect;
        v3 position = dragging.start_position + offset;

        transform.position.z = math::min(transform.position.z + raise_speed * Input::delta_time, 0.5f);
        transform.position.xy = position.xy;
    }
}

void collision_update_system(Scene* scene) {
    ZoneScoped;

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

void roller_system(Scene* scene) {
    ZoneScoped;
    auto rollers = scene->registry.view<Roller, ModelTransform>();

    static bool generated = false;
    if (!generated) {
        MaterialCPU material_cpu = { .file_path = "rollee_material", .color_tint = palette::slate_gray};
        game.renderer.upload_material(material_cpu);
        game.renderer.upload_mesh(generate_icosphere(3));
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
            scene->registry.emplace<ModelTransform>(new_entity, transform.translation + v3(0.2f * dirs[i], 0.0f), euler(), roller.rollee_radius);
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

        auto p_transform = scene->registry.try_get<ModelTransform>(entity);
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
