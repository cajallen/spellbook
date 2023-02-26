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
    Color ambient             = Color(palette::white, 0.15);
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
    v4 outline = v4(0.10f, 0.50f, 0.1f, 1.0f);
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
    
    vuk::Buffer buffer_composite_data;

    v2i         query = v2i(-1, -1);
    vuk::Future fut_query_result;

    bool user_pause = false;
    bool cull_pause = false;
    vuk::Texture render_target;

    bool render_grid = true;
    bool render_widgets = true;
    
    void update_size(v2i new_size);
    
    void        setup(vuk::Allocator& allocator);
    void        image(v2i size);
    void        settings_gui();
    void        pre_render();
    void        update();
    vuk::Future render(vuk::Allocator& allocator, vuk::Future target);

    void add_sundepth_pass(std::shared_ptr<vuk::RenderGraph> rg);
    void add_forward_pass(std::shared_ptr<vuk::RenderGraph> rg);
    void add_widget_pass(std::shared_ptr<vuk::RenderGraph> rg);
    void add_postprocess_pass(std::shared_ptr<vuk::RenderGraph> rg);
    void add_info_read_pass(std::shared_ptr<vuk::RenderGraph> rg);

    void prune_emitters();

    void        cleanup(vuk::Allocator& allocator);

    Renderable* add_renderable(Renderable&& renderable);
    Renderable* add_renderable(const Renderable& renderable);
    Renderable* copy_renderable(Renderable* renderable);
    void        delete_renderable(Renderable* renderable);

    Renderable& quick_mesh(const MeshCPU& mesh_cpu, bool frame_allocated, bool widget = false);
    Renderable& quick_mesh(const string& mesh_name, bool frame_allocated, bool widget = false);
    Renderable& quick_mesh(const string& mesh_name, const string& mat_name, bool frame_allocated);
    Renderable& quick_material(const MaterialCPU& material_cpu, bool frame_allocated);

    void _upload_buffer_objects(vuk::Allocator& frame_allocator);

    vuk::Buffer buffer_model_mats;
    vuk::Buffer buffer_ids;
    umap<mat_id, umap<mesh_id, vector<std::tuple<u32, m44GPU*>>>> renderables_built;
    umap<mat_id, umap<mesh_id, vector<std::tuple<u32, SkeletonGPU*, m44GPU*>>>> rigged_renderables_built;
    void setup_renderables_for_passes(vuk::Allocator& allocator);
};

}
