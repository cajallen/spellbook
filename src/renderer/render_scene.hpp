#pragma once

#include <vuk/Buffer.hpp>

#include "string.hpp"

#include "slotmap.hpp"
#include "viewport.hpp"
#include "renderer.hpp"
#include "renderer/renderable.hpp"


namespace spellbook {

struct Renderable;

struct SceneData {
    v4  ambient             = v4(1.0f, 1.0f, 1.0f, 0.3f);
    v3  fog_color           = v3(0.04f, 0.02f, 0.04f);
    f32 fog_depth           = -1.0f;
    v2  rim_intensity_start = v2(0.25f, 0.55f);
    v3  sun_direction       = v3(0.5f, 0.5f, 0.5f);
    f32 sun_intensity       = 1.0f;
};

struct PostProcessData {
    v4 outline = v4(5.0f, 6.0f, 0.05f, 3.0f);
};

struct RenderScene {
    string              name;

    plf::colony<Renderable> renderables;
    Viewport                viewport;

    SceneData       scene_data;
    PostProcessData post_process_data;

    vuk::BufferCrossDevice buffer_camera_data;
    vuk::BufferCrossDevice buffer_scene_data;
    vuk::BufferCrossDevice buffer_model_mats;

    v2i         query = v2i(-1, -1);
    vuk::Future fut_query_result;

    bool pause = false;
    vuk::Texture render_target;

    void update_size(v2i new_size);
    
    void        setup(vuk::Allocator& allocator);
    void        image(v2i size);
    void        settings_gui();
    void        pre_render();
    vuk::Future render(vuk::Allocator& allocator, vuk::Future target);
    void        cleanup(vuk::Allocator& allocator);

    Renderable* add_renderable(Renderable renderable);
    Renderable* copy_renderable(Renderable* renderable);
    void        delete_renderable(Renderable* renderable);
    
    void _upload_buffer_objects(vuk::Allocator& frame_allocator);
};

}
