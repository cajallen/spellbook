#pragma once
#include <optional>
#include <mutex>

#include <vuk/Allocator.hpp>
#include <vuk/Context.hpp>
#include <vuk/RenderGraph.hpp>
#include <vuk/SampledImage.hpp>
#include <vuk/resources/DeviceFrameResource.hpp>
#include <VkBootstrap.h>

#include "lib_ext/glfw.hpp"
#include "lib_ext/vuk_imgui.hpp"

#include "vector.hpp"
#include "string.hpp"

#include "matrix.hpp"
#include "umap.hpp"
#include "assets/model.hpp"

#include "renderer/assets/mesh.hpp"
#include "renderer/assets/material.hpp"
#include "renderer/assets/texture.hpp"

using std::optional;

namespace spellbook {

struct FrameTimer {
    int               ptr         = 0;
    int               filled      = 0;
    array<float, 200> frame_times = {};
    array<float, 200> delta_times = {};

    void update();
    void inspect();
};

struct RenderScene;

struct Renderer {
    FrameTimer           frame_timer;
    vector<RenderScene*> scenes;

    bool setup_finished = false;

    enum RenderStage { RenderStage_Inactive, RenderStage_BuildingRG, RenderStage_Presenting };

    v2i                                     window_size = {1920, 1080};
    VkDevice                                device;
    VkPhysicalDevice                        physical_device;
    VkQueue                                 graphics_queue;
    VkQueue                                 transfer_queue;
    optional<vuk::Context>                  context;
    optional<vuk::DeviceSuperFrameResource> super_frame_resource;
    optional<vuk::Allocator>                global_allocator;
    optional<vuk::Allocator>                frame_allocator;
    vuk::SwapchainRef                       swapchain;
    GLFWwindow*                             window;
    VkSurfaceKHR                            surface;
    vkb::Instance                           vkbinstance;
    vkb::Device                             vkbdevice;
    ImGuiData                               imgui_data;
    std::vector<vuk::Future>                futures;
    std::mutex                              setup_lock;

    plf::colony<vuk::SampledImage> sampled_images;

    umap<string, u64> mesh_aliases;
    umap<string, u64> material_aliases;
    umap<string, u64> texture_aliases;
    umap<u64, MeshGPU>     mesh_cache;
    umap<u64, MaterialGPU> material_cache;
    umap<u64, TextureGPU>  texture_cache;

    RenderStage stage = RenderStage_Inactive;

    vector<std::function<void()>> start_render_callbacks;
    
    void enqueue_setup(vuk::Future&& fut) {
        std::scoped_lock _(setup_lock);
        futures.emplace_back(std::move(fut));
    }

    Renderer();
    void setup();
    void update();
    void render();
    void cleanup();
    ~Renderer();

    void wait_for_futures();

    void add_scene(RenderScene*);

    // Uploads asset, stores in alias and cache, overwrites old cache entry
    MeshGPU&     upload_mesh(const MeshCPU&, bool frame_allocation = false);
    MaterialGPU& upload_material(const MaterialCPU&, bool frame_allocation = false);
    TextureGPU&  upload_texture(const TextureCPU&, bool frame_allocation = false);

    // Returns an asset given the path of the .sb*** asset, nullptr if not uploaded
    MeshGPU* get_mesh(const string& asset_path);
    MaterialGPU* get_material(const string& asset_path);
    TextureGPU* get_texture(const string& asset_path);

    // Returns an asset given the path of the .sb*** asset, uploads and returns if not uploaded
    MeshGPU& get_mesh_or_upload(const string& asset_path);
    MaterialGPU& get_material_or_upload(const string& asset_path);
    TextureGPU& get_texture_or_upload(const string& asset_path);

    void generate_thumbnail(const ModelCPU& model, const string& name);
    
    void upload_defaults();

    void resize(v2i new_size);

    void debug_window(bool* p_open);
};

}
