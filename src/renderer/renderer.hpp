#pragma once

#include <optional>
#include <mutex>
#include <array>

#include <vuk/resources/DeviceFrameResource.hpp>
#include <vuk/Context.hpp>
#include <vuk/RenderGraph.hpp>
#include <VkBootstrap.h>

#include "extension/vuk_imgui.hpp"
#include "general/vector.hpp"
#include "general/math/geometry.hpp"
#include "renderer/assets/material.hpp"
#include "renderer/assets/mesh.hpp"
#include "renderer/assets/texture.hpp"

struct GLFWwindow;

using std::optional;

namespace spellbook {

#define CAMERA_BINDING 0
#define MODEL_BINDING 1
#define ID_BINDING 2
#define BONES_BINDING 3
#define MATERIAL_BINDING 4
#define BASE_COLOR_BINDING 5
#define ORM_BINDING 6
#define NORMAL_BINDING 7
#define EMISSIVE_BINDING 8
#define SPARE_BINDING_1 9
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

    vuk::Compiler compiler;

    ImGuiData                      imgui_data;
    plf::colony<vuk::SampledImage> imgui_images;
    char* imgui_ini_path;

    vuk::Unique<array<VkSemaphore, 3>> present_ready;
    vuk::Unique<array<VkSemaphore, 3>> render_complete;

    bool suspend = false;
    
    Renderer();
    void setup();
    void update();
    void render();
    void cleanup();
    ~Renderer();

    void enqueue_setup(vuk::Future&& fut);
    void wait_for_futures();

    void add_scene(RenderScene*);

    void resize(v2i new_size);

    void debug_window(bool* p_open);
};

Renderer& get_renderer();

}
