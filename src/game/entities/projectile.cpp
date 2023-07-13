#include "projectile.hpp"

#include "general/math/matrix_math.hpp"
#include "game/scene.hpp"
#include "game/entities/components.hpp"

namespace spellbook {

entt::entity quick_projectile(Scene* scene, Projectile proj, v3 pos, const string& particles_path, const string& model_path, float scale) {
    auto       entity = scene->registry.create();
    scene->registry.emplace<Name>(entity, "projectile");

    scene->registry.emplace<LogicTransform>(entity, pos);
    
    if (!model_path.empty()) {
        auto& model_comp = scene->registry.emplace<Model>(entity);
        model_comp.model_cpu = std::make_unique<ModelCPU>(load_asset<ModelCPU>(model_path));
        model_comp.model_gpu = instance_model(scene->render_scene, *model_comp.model_cpu);

        scene->registry.emplace<ModelTransform>(entity, v3{}, quat{}, v3(scale));
        scene->registry.emplace<TransformLink>(entity, v3(0.5), 10.0f);
    }

    auto& emitter_comp = scene->registry.emplace<EmitterComponent>(entity, scene);
    emitter_comp.add_emitter(0, load_asset<EmitterCPU>(particles_path));
    
    scene->registry.emplace<Projectile>(entity, proj);
    
    return entity;
}

void projectile_system(Scene* scene) {
    auto projectile_view = scene->registry.view<Projectile, LogicTransform>();
    for (auto [entity, projectile, logic_tfm] : projectile_view.each()) {
        // Velocity and movement 
        v3 vec_to = v3(projectile.target) - logic_tfm.position;
        v3 velocity_dir = math::normalize(vec_to);
        v3 velocity = velocity_dir * math::min(projectile.speed.value() * scene->delta_time, math::length(vec_to));
        logic_tfm.position += velocity;

        // Alignment
        if (math::length(projectile.alignment) > 0.0f) {
            logic_tfm.yaw = math::angle_difference(projectile.alignment.xy, velocity_dir.xy);
            if (projectile.first_frame) {
                auto model_tfm = scene->registry.try_get<ModelTransform>(entity);
                if (model_tfm)
                    model_tfm->set_rotation(math::to_quat(math::normal_yaw(logic_tfm.normal, logic_tfm.yaw)));
            }
        }

        // Trigger
        float dist = math::distance(v3(projectile.target), logic_tfm.position);
        if (dist < 0.1f) {
            projectile.callback(entity);
            
            scene->registry.emplace<Killed>(entity);
        }
        projectile.first_frame = false;
    }
}

}