#pragma once
#include <optional>
#include <mutex>

#include <vuk/Allocator.hpp>
#include <vuk/Context.hpp>
#include <vuk/RenderGraph.hpp>
#include <vuk/SampledImage.hpp>
#include <vuk/resources/DeviceFrameResource.hpp>
#include <VkBootstrap.h>

#include "extension/glfw.hpp"
#include "extension/vuk_imgui.hpp"
#include "general/string.hpp"
#include "general/vector.hpp"
#include "general/matrix.hpp"
#include "general/umap.hpp"
#include "renderer/assets/model.hpp"
#include "renderer/assets/mesh.hpp"
#include "renderer/assets/material.hpp"
#include "renderer/assets/texture.hpp"

using std::optional;

namespace spellbook {

#define CAMERA_BINDING 0
#define MODEL_BINDING 1
#define BONES_BINDING 2
#define MATERIAL_BINDING 3
#define BASE_COLOR_BINDING 4
#define ORM_BINDING 5
#define NORMAL_BINDING 6
#define EMISSIVE_BINDING 7
#define SPARE_BINDING_1 8
#define PARTICLES_BINDING MODEL_BINDING

struct FrameTimer {
    int               ptr         = 0;
    int               filled      = 0;
    array<float, 200> frame_times = {};
    array<float, 200> delta_times = {};

    void update();
    void inspect();
};

struct RenderScene;

enum RenderStage { RenderStage_Inactive, RenderStage_Setup, RenderStage_BuildingRG, RenderStage_Presenting };

struct Renderer {
    RenderStage stage = RenderStage_Setup;
    FrameTimer           frame_timer;
    vector<RenderScene*> scenes;

    v2i                                     window_size = {2560, 1440};
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
    std::vector<vuk::Future>                futures;
    std::mutex                              setup_lock;

    ImGuiData                      imgui_data;
    plf::colony<vuk::SampledImage> imgui_images;

    umap<string, u64> mesh_aliases;
    umap<string, u64> material_aliases;
    umap<string, u64> texture_aliases;
    umap<u64, MeshGPU>     mesh_cache;
    umap<u64, MaterialGPU> material_cache;
    umap<u64, TextureGPU>  texture_cache;

    bool suspend = false;
    
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

    // Returns an asset given the path of the .sb*** asset, nullptr if not uploaded
    MeshGPU* get_mesh(const string& asset_path);
    MaterialGPU* get_material(const string& asset_path);
    TextureGPU* get_texture(const string& asset_path);

    // Returns an asset given the path of the .sb*** asset, uploads and returns if not uploaded
    MeshGPU& get_mesh_or_upload(const string& asset_path);
    MaterialGPU& get_material_or_upload(const string& asset_path);
    TextureGPU& get_texture_or_upload(const string& asset_path);
    
    void upload_defaults();

    void resize(v2i new_size);

    void debug_window(bool* p_open);
};

}
