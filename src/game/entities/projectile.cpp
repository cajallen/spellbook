#include "projectile.hpp"

#include "general/matrix_math.hpp"
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

        scene->registry.emplace<ModelTransform>(entity, v3{}, euler{}, v3(scale));
        scene->registry.emplace<TransformLink>(entity, v3(0.5));
    }

    scene->registry.emplace<EmitterComponent>(entity, 
        &instance_emitter(scene->render_scene, load_asset<EmitterCPU>(particles_path)), nullptr
    );

    scene->registry.emplace<Projectile>(entity, proj);
    
    return entity;
}

void projectile_system(Scene* scene) {
    auto projectile_view = scene->registry.view<Projectile, LogicTransform>();
    for (auto [entity, projectile, l_transform] : projectile_view.each()) {
        // Velocity and movement 
        v3 vec_to = v3(projectile.target) - l_transform.position;
        v3 velocity_dir = math::normalize(vec_to);
        v3 velocity = velocity_dir * math::min(projectile.speed.value() * scene->delta_time, math::length(vec_to));
        l_transform.position += velocity;

        // Alignment
        auto m_transform = scene->registry.try_get<ModelTransform>(entity);
        if (m_transform && math::length(projectile.alignment) > 0.0f) {
            v3 vr = velocity_dir - math::dot(projectile.alignment, velocity_dir) * projectile.alignment;
            v3 basis_2 = math::normalize(vr);
            v3 basis_3 = math::normalize(math::cross(velocity_dir, projectile.alignment));
            v3 basis_1 = math::normalize(math::cross(basis_2, basis_3));
            m44 rotation = m44(basis_1.x, basis_2.x, basis_3.x, basis_1.y, basis_2.y, basis_3.y, basis_1.z, basis_2.z, basis_3.z);
            
            m_transform->set_rotation(math::to_euler(rotation));
        }

        // Trigger
        float dist = math::distance(v3(projectile.target), l_transform.position);
        if (dist < 0.1f) {
            projectile.callback(entity, projectile.payload);
            if (projectile.payload_owned)
                free(projectile.payload);
            
            scene->registry.emplace<Killed>(entity);
        }
    }
}

}