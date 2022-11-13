#pragma once

#include <vuk/Buffer.hpp>

#include "string.hpp"

#include "slotmap.hpp"
#include "viewport.hpp"
#include "renderer.hpp"
#include "assets/particles.hpp"
#include "renderer/renderable.hpp"


namespace spellbook {

struct Renderable;

struct SceneData {
    Color ambient             = Color(palette::white, 0.05);
    Color fog_color           = palette::black;
    f32 fog_depth             = -1.0f;
    v3  rim_alpha_width_start = v3(0.25f, 0.2f, 0.6f);
    v3  sun_direction         = v3(-0.3f, 0.6f, 0.1f);
    f32 sun_intensity         = 1.0f;
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

    vector<ParticleEmitter> emitters;
    
    void update_size(v2i new_size);
    
    void        setup(vuk::Allocator& allocator);
    void        image(v2i size);
    void        settings_gui();
    void        pre_render();
    vuk::Future render(vuk::Allocator& allocator, vuk::Future target);
    void        cleanup(vuk::Allocator& allocator);

    Renderable* add_renderable(Renderable&& renderable);
    Renderable* add_renderable(const Renderable& renderable);
    Renderable* copy_renderable(Renderable* renderable);
    void        delete_renderable(Renderable* renderable);

    void quick_mesh(const MeshCPU& mesh_cpu);
    
    void _upload_buffer_objects(vuk::Allocator& frame_allocator);
};

}
