#pragma once

#include "camera.hpp"
#include "console.hpp"
#include "geometry.hpp"

namespace spellbook {

struct Viewport {
    string  name;
    Camera* camera;
    v2i     start;
    v2i     size;
    bool    size_dirty = true;
    void    update_size(v2i new_size);
    m44     proj_2d = {};

    bool hovered        = false;
    bool window_hovered = false;
    bool focused        = false;

    void pre_render();
    f32  aspect_xy();

    void setup();
    r3   ray(v2i screen_pos);
};

void inspect(Viewport* viewport);

}
