#pragma once

#include "general/geometry.hpp"
#include "general/matrix.hpp"
#include "general/json.hpp"

namespace spellbook {

struct Camera {
    float fov         = 30.0f;
    v2    clip_planes = {0.2f, 30.0f};
    float aspect_xy   = 1920.0f / 1080.0f;

    v3    position;
    euler heading; // euler_rad

    m44 view;
    m44 proj;
    m44 vp;

    bool view_dirty = true;
    bool proj_dirty = true;

    Camera() = default;
    Camera(v3 pos, euler rot);

    void update_proj();
    void update_view();

    void set_aspect_xy(float new_aspect_xy);

    void pre_render();
};

JSON_IMPL(Camera, position, heading);

void inspect(Camera* camera);

}
