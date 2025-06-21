#pragma once

#include <optional>
#include <mutex>
#include <array>

#include <vuk/Value.hpp>
#include <vuk/RenderGraph.hpp>

#include "general/memory.hpp"
#include "general/function.hpp"
#include "general/math/geometry.hpp"
#include "extension/vuk_imgui.hpp"

struct GLFWwindow;

using std::optional;
using std::array;

namespace spellbook {

// standard
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

// font
#define VIEW_BINDING 0
#define ATLAS_BINDING 1

struct FrameTimer {
    int32             current_frame = 1;
    int32             ptr         = 0;
    int32             filled      = 0;
    array<float, 200> frame_times = {};
    array<float, 200> delta_times = {};

    void update();
    void inspect();
};

struct RenderScene;
struct FontManager;
struct Runtime;

enum RenderStage { RenderStage_Inactive, RenderStage_Setup, RenderStage_BuildingRG, RenderStage_Presenting };

struct Renderer {
    RenderStage stage = RenderStage_Setup;
    FrameTimer           frame_timer;
    vector<RenderScene*> scenes;

    vector<function<void(void)>> extra_passes;
    bool do_extra_passes = false;

    unique_ptr<Runtime> runtime;
    ImGuiData imgui_data;

    v2i                      window_size = {2560, 1440};
    VkQueue                  graphics_queue;
    VkQueue                  transfer_queue;
    optional<vuk::Allocator> global_allocator;
    optional<vuk::Allocator> frame_allocator;
    GLFWwindow*              window;

    vector<vuk::UntypedValue>  futures;
    std::mutex                 setup_lock;

    vuk::Compiler compiler;

    bool suspend = false;
    
    Renderer();
    void setup();
    void update();
    void render();
    void present();
    void cleanup();
    ~Renderer();

    void enqueue_setup(vuk::UntypedValue fut);
    void wait_for_futures();

    void add_scene(RenderScene*);

    void resize(v2i new_size);

    void debug_window(bool* p_open);
};

Renderer& get_renderer();

}
