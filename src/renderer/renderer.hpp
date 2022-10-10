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

    v2i                                       window_size = {1920, 1080};
    VkDevice                                  device;
    VkPhysicalDevice                          physical_device;
    VkQueue                                   graphics_queue;
    VkQueue                                   transfer_queue;
    optional<vuk::Context>                  context;
    optional<vuk::DeviceSuperFrameResource> super_frame_resource;
    optional<vuk::Allocator>                global_allocator;
    optional<vuk::Allocator>                frame_allocator;
    vuk::SwapchainRef                       swapchain;
    GLFWwindow*                               window;
    VkSurfaceKHR                              surface;
    vkb::Instance                             vkbinstance;
    vkb::Device                               vkbdevice;
    ImGuiData                                 imgui_data;
    vector<vuk::Future>                     futures;
    std::mutex                                setup_lock;

    plf::colony<vuk::SampledImage> sampled_images;
    
    // MEMORY: replace CPU storage here with some hashed version
    umap<MeshCPU, MeshGPU>         mesh_cache;
    umap<MaterialCPU, MaterialGPU> material_cache;
    umap<TextureCPU, TextureGPU>   texture_cache;

    RenderStage stage = RenderStage_Inactive;

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

    void add_scene(RenderScene*);

    MeshGPU& upload_mesh(const MeshCPU&, bool frame_allocation = false);
    MaterialGPU& upload_material(const MaterialCPU&, bool frame_allocation = false);
    TextureGPU& upload_texture(const TextureCPU&, bool frame_allocation = false);

    MeshGPU* find_mesh(string_view name);
    MaterialGPU* find_material(string_view name);
    TextureGPU* find_texture(string_view name);

    void resize(v2i new_size);

    void debug_window(bool* p_open);
};

}
