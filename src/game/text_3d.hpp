#pragma once

#include <entt/entity/fwd.hpp>
#include "general/math/geometry.hpp"

namespace spellbook {

struct Renderable;
struct Scene;

struct Text3D {
    Text3D(Scene* s) : scene(s) {}

    void set_text(string_view text);
    string_view get_text() const { return text; }

    vector<Renderable*> renderables;
 protected:
    Scene* scene;
    string text;
};

struct Text3DTransform {
    enum BillboardMode {
        BillboardMode_Absolute,
        BillboardMode_None,
        BillboardMode_Z,
        BillboardMode_X
    };
    v3 origin;
    v3 offset;
    float scale = 1.0f;

    BillboardMode billboard_mode = BillboardMode_Absolute;
    v3 alignment_axis; // X-axis will be locked to this if mode=X

    float lifetime = -1.0f; // 0-1
};

void text_3d_system(Scene* scene);
void on_text_3d_destroy(Scene& scene, entt::registry& registry, entt::entity entity);


}