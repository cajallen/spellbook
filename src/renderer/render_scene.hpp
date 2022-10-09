#pragma once

#include "vector.hpp"
#include "string.hpp"

#include "renderable.hpp"
#include "viewport.hpp"
#include "renderer.hpp"
#include "slotmap.hpp"
#include "renderer/assets/material.hpp"
#include "renderer/assets/mesh.hpp"
#include "renderer/assets/texture.hpp"

#include "vuk/Buffer.hpp"

namespace spellbook {

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
    vector<MeshCPU>     mesh_dependencies;
    vector<MaterialCPU> material_dependencies;
    vector<TextureCPU>  texture_dependencies;

    slotmap<Renderable> renderables;
    vector<Renderable>  frame_renderables;
    Viewport            viewport;

    SceneData       scene_data;
    PostProcessData post_process_data;

    vuk::BufferCrossDevice buffer_camera_data;
    vuk::BufferCrossDevice buffer_scene_data;
    vuk::BufferCrossDevice buffer_model_mats;

    v2i           query = v2i(-1, -1);
    vuk::Future fut_query_result;

    void _upload_dependencies();
    void _upload_buffer_objects(vuk::Allocator& frame_allocator);

    void          setup(vuk::Allocator& allocator);
    vuk::Future render(vuk::Allocator& allocator, vuk::Future future);
    void          cleanup(vuk::Allocator& allocator);

    slot<Renderable> add_renderable(Renderable renderable);
    slot<Renderable> copy_renderable(slot<Renderable> renderable);
    void             delete_renderable(slot<Renderable> renderable);
};

void inspect(RenderScene* render_scene);

}
