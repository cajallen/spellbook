#include "text_3d.hpp"

#include "game/scene.hpp"
#include "game/effects/text_layer.hpp"
#include "game/entities/components.hpp"
#include "general/math/matrix_math.hpp"

#include "editor/console.hpp"

namespace spellbook {

Text3D::~Text3D() {
    while (!renderables.empty()) {
        Renderable* renderable = renderables.front();
        auto it = scene->render_scene.ui_renderables.get_iterator(renderable);
        if (it != scene->render_scene.ui_renderables.end())
            scene->render_scene.ui_renderables.erase(it);
        renderables.remove_index(0);
    }
}

void Text3D::set_text(string_view new_text) {
    if (text == new_text)
        return;
    text = new_text;
    while (!renderables.empty()) {
        Renderable* renderable = renderables.front();
        scene->render_scene.ui_renderables.erase(scene->render_scene.ui_renderables.get_iterator(renderable));
        renderables.remove_index(0);
    }

    range2i region = calc_text_region(vector<string>{string(text)}, vector<Font*>{get_game_text_layer().spatial_font}, v2i{});
    TextSettings text_settings = {
        .spatial = true,
        .position = -region.end / 2,
        .depth = -1
    };
    renderables = upload_text_mesh(vector<string>{string(text)}, vector<Font*>{get_game_text_layer().spatial_font}, scene->render_scene, text_settings);
}

void text_3d_system(Scene* scene) {

    auto text_rise_system = scene->registry.view<Text3DTransform>();
    for (auto [entity, text_tfm] : text_rise_system.each()) {
        if (text_tfm.lifetime < 0.0f)
            continue;

        if (text_tfm.lifetime > 1.0)
            scene->registry.emplace<Killed>(entity);

        text_tfm.lifetime += scene->delta_time / 1.0f;
    }
    auto text_3d_transform_system = scene->registry.view<Text3DTransform, Text3D>();
    for (auto [entity, text_tfm, text_3d] : text_3d_transform_system.each()) {
        for (Renderable* r : text_3d.renderables) {
            m44 to_billboarded;

            if (text_tfm.billboard_mode == Text3DTransform::BillboardMode_Absolute) {
                v3 up_axis = v3::Z;
                v3 dimensional_axis = math::euler2vector(scene->camera.heading);
                v3 right_axis = math::normalize(math::cross(dimensional_axis, up_axis));
                up_axis = math::normalize(math::cross(right_axis, dimensional_axis));

                to_billboarded = {
                    right_axis.x, -up_axis.x, dimensional_axis.x,
                    right_axis.y, -up_axis.y, dimensional_axis.y,
                    right_axis.z, -up_axis.z, dimensional_axis.z
                };
            } else if (text_tfm.billboard_mode == Text3DTransform::BillboardMode_Z) {
                v3 up_axis = v3::Z;
                v3 dimensional_axis = math::euler2vector(scene->camera.heading);
                v3 right_axis = math::normalize(math::cross(dimensional_axis, up_axis));
                dimensional_axis = math::normalize(math::cross(up_axis, right_axis));

                to_billboarded = {
                    right_axis.x, -up_axis.x, dimensional_axis.x,
                    right_axis.y, -up_axis.y, dimensional_axis.y,
                    right_axis.z, -up_axis.z, dimensional_axis.z
                };
            } else if (text_tfm.billboard_mode == Text3DTransform::BillboardMode_X) {
                v3 right_axis = math::normalize(text_tfm.alignment_axis);
                v3 dimensional_axis = math::euler2vector(scene->camera.heading);
                v3 up_axis = math::normalize(math::cross(right_axis, dimensional_axis));

                to_billboarded = {
                    right_axis.x, -up_axis.x, dimensional_axis.x,
                    right_axis.y, -up_axis.y, dimensional_axis.y,
                    right_axis.z, -up_axis.z, dimensional_axis.z
                };
            } else {
                to_billboarded = m33::identity();
            }

            v3 rise = text_tfm.lifetime > 0.0f ? v3(0.0f, 0.0f, math::pow(text_tfm.lifetime, 3.0f) * 0.5f) : v3();
            m44 transform = math::translate(text_tfm.origin + rise) * to_billboarded * math::translate(text_tfm.offset) * math::scale(v3(0.001f * text_tfm.scale));
            r->transform = m44GPU(transform);
        }
    }
}

};