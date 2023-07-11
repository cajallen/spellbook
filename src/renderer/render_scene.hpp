#pragma once

#include <plf_colony.h>
#include <vuk/vuk_fwd.hpp>
#include <vuk/Buffer.hpp>
#include <vuk/Image.hpp>
#include <vuk/Future.hpp>

#include "general/string.hpp"
#include "general/geometry.hpp"
#include "general/quaternion.hpp"
#include "general/color.hpp"

#include "renderer/viewport.hpp"
#include "renderer/renderable.hpp"
#include "renderer/assets/particles.hpp"

namespace spellbook {

using mesh_id = u64;
using mat_id = u64;

struct MeshCPU;
struct MaterialCPU;
struct SkeletonGPU;

struct SceneData {
    Color ambient;
    Color fog_color;
    f32 fog_depth;
    v3  rim_alpha_width_start;
    quat sun_direction;
    f32 sun_intensity;
    v3 water_color1;
    v3 water_color2;
    float water_intensity;
    float water_level;
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
    v4 outline = v4(0.01f, 0.20f, 0.01f, 0.10f);
    DebugDrawMode debug_mode = DebugDrawMode_Lit;
    float time;
};

struct RenderScene {
    string              name;
    
    plf::colony<StaticRenderable> static_renderables;
    plf::colony<Renderable> renderables;
    plf::colony<Renderable> widget_renderables;
    plf::colony<EmitterGPU> emitters;
    Viewport                viewport;

    SceneData       scene_data;
    PostProcessData post_process_data;
    
    v2i         query = v2i(-1, -1);
    vuk::Future fut_query_result;

    bool user_pause = false;
    bool cull_pause = false;
    vuk::Texture render_target;

    bool render_grid = false;
    bool render_widgets = true;

    vuk::Buffer buffer_camera_data;
    vuk::Buffer buffer_sun_camera_data;
    vuk::Buffer buffer_top_camera_data;
    vuk::Buffer buffer_composite_data;
    
    void update_size(v2i new_size);
    
    void        setup(vuk::Allocator& allocator);
    void        image(v2i size);
    void        settings_gui();
    void        pre_render();
    void        update();
    vuk::Future render(vuk::Allocator& allocator, vuk::Future target);

    void add_sundepth_pass(std::shared_ptr<vuk::RenderGraph> rg);
    void add_topdepth_pass(std::shared_ptr<vuk::RenderGraph> rg);
    void add_topdepth_blur_pass(std::shared_ptr<vuk::RenderGraph> rg);
    void add_forward_pass(std::shared_ptr<vuk::RenderGraph> rg);
    void add_widget_pass(std::shared_ptr<vuk::RenderGraph> rg);
    void add_postprocess_pass(std::shared_ptr<vuk::RenderGraph> rg);
    void add_info_read_pass(std::shared_ptr<vuk::RenderGraph> rg);
    void add_emitter_update_pass(std::shared_ptr<vuk::RenderGraph> rg);

    void prune_emitters();

    void        cleanup(vuk::Allocator& allocator);

    Renderable* add_renderable(const Renderable& renderable);
    void        delete_renderable(Renderable* renderable);
    void        delete_renderable(StaticRenderable* renderable);

    Renderable& quick_mesh(const MeshCPU& mesh_cpu, bool frame_allocated, bool widget);
    Renderable& quick_material(const MaterialCPU& material_cpu, bool frame_allocated);
    Renderable& quick_renderable(const MeshCPU& mesh_id, u64 mat_id, bool frame_allocated);
    Renderable& quick_renderable(u64 mesh_id, u64 mat_id, bool frame_allocated);
    Renderable& quick_renderable(u64 mesh_id, const MaterialCPU& mat_id, bool frame_allocated);

    void _upload_buffer_objects(vuk::Allocator& frame_allocator);

    vuk::Buffer buffer_model_mats;
    vuk::Buffer buffer_ids;

    umap<mat_id, umap<mesh_id, vector<std::tuple<u32, m44GPU*>>>> renderables_built;
    umap<mat_id, umap<mesh_id, vector<std::tuple<u32, SkeletonGPU*, m44GPU*>>>> rigged_renderables_built;
    void setup_renderables_for_passes(vuk::Allocator& allocator);
};

}
