#pragma once

#include <vuk/Buffer.hpp>

#include "light.hpp"
#include "general/string.hpp"

#include "renderer/viewport.hpp"
#include "renderer/renderer.hpp"
#include "renderer/assets/particles.hpp"
#include "renderer/renderable.hpp"


namespace spellbook {

struct Renderable;

struct SceneData {
    Color ambient             = Color(palette::white, 0.05);
    Color fog_color           = palette::black;
    f32 fog_depth             = -1.0f;
    v3  rim_alpha_width_start = v3(0.1f, 0.1f, 0.75f);
    quat sun_direction        = quat(0.2432103, 0.3503661, 0.0885213, 0.9076734);
    f32 sun_intensity         = 1.0f;
};

enum DebugDrawMode {
    DebugDrawMode_Lit,
    DebugDrawMode_BaseColor,
    DebugDrawMode_Emissive,
    DebugDrawMode_Position,
    DebugDrawMode_Normal,
    DebugDrawMode_Depth,
    DebugDrawMode_None
};

struct PostProcessData {
    v4 outline = v4(0.10f, 0.30f, 0.1f, 1.0f);
    DebugDrawMode debug_mode = DebugDrawMode_Lit;
};

struct RenderScene {
    string              name;

    plf::colony<Renderable> renderables;
    plf::colony<Renderable> widget_renderables;
    plf::colony<EmitterGPU> emitters;
    Viewport                viewport;

    SceneData       scene_data;
    PostProcessData post_process_data;

    vuk::Buffer buffer_camera_data;
    vuk::Buffer buffer_sun_camera_data;
    vuk::Buffer buffer_model_mats;
    
    vuk::Buffer buffer_composite_data;

    v2i         query = v2i(-1, -1);
    vuk::Future fut_query_result;

    bool user_pause = false;
    bool cull_pause = false;
    vuk::Texture render_target;


    vector<LightGPU> submitted_lights;

    bool render_grid = true;
    bool render_widgets = true;
    
    void update_size(v2i new_size);
    
    void        setup(vuk::Allocator& allocator);
    void        image(v2i size);
    void        settings_gui();
    void        pre_render();
    void        update();
    vuk::Future render(vuk::Allocator& allocator, vuk::Future target);
    void        cleanup(vuk::Allocator& allocator);

    Renderable* add_renderable(Renderable&& renderable);
    Renderable* add_renderable(const Renderable& renderable);
    Renderable* copy_renderable(Renderable* renderable);
    void        delete_renderable(Renderable* renderable);

    Renderable& quick_mesh(const MeshCPU& mesh_cpu, bool frame_allocated, bool widget = false);
    Renderable& quick_mesh(const string& mesh_name, bool frame_allocated, bool widget = false);
    Renderable& quick_material(const MaterialCPU& material_cpu, bool frame_allocated);

    void _upload_buffer_objects(vuk::Allocator& frame_allocator);
};

}
