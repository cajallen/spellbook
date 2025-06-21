#include "renderer.hpp"

#include <backends/imgui_impl_glfw.h>
#include <tracy/Tracy.hpp>
#include <stb_image.h>

#include <vuk/vsl/Core.hpp>

#include "extension/glfw.hpp"
#include "extension/fmt.hpp"
#include "extension/fmt_geometry.hpp"
#include "extension/imgui_extra.hpp"
#include "general/hash.hpp"
#include "general/math/math.hpp"
#include "general/logger.hpp"
#include "general/input.hpp"
#include "renderer/draw_functions.hpp"
#include "renderer/render_scene.hpp"
#include "renderer/sampler.hpp"
#include "renderer/gpu_asset_cache.hpp"
#include "renderer/runtime.hpp"

#include "extension/vuk_imgui.hpp"

namespace spellbook {

Renderer& get_renderer() {
    static Renderer renderer;
    return renderer;
}

Renderer::Renderer() : imgui_data() {
	vkb::InstanceBuilder instance_builder = make_instance_builder(1, 2, true).set_app_name("saurian").set_engine_name("spellbook").set_app_version(0, 1, 0);
	auto inst_ret = instance_builder.build();
	if (!inst_ret) {
		auto err_msg = std::string("ERROR: couldn't initialise instance - ") + inst_ret.error().message();
		printf("ERROR: %s\n", err_msg.c_str());
		throw std::runtime_error(err_msg);
	}
	vkb::Instance instance = inst_ret.value();

	window = create_window_glfw("Spellbook", v2i{1920, 1080}, false);
	glfwSetWindowUserPointer(window, this);
	auto surface = create_surface_glfw(instance.instance, window);
	auto physical_device = select_physical_device(instance, surface);
	runtime = make_device_builder(physical_device).set_recommended_features().build_app(true, 3);

    global_allocator.emplace(*runtime->superframe_allocator);
}

void Renderer::enqueue_setup(vuk::UntypedValue fut) {
    std::scoped_lock _(setup_lock);
    futures.emplace_back(std::move(fut));
}

void Renderer::add_scene(RenderScene* scene) {
    scenes.push_back(scene);
    if (stage != RenderStage_Setup) {
        scene->setup(*global_allocator);
    }
}

void Renderer::setup() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsSpellbook();
    static char imgui_ini_path[256];
    strcpy(imgui_ini_path, ("imgui.ini"_config).abs_string().c_str());
    ImGui::GetIO().IniFilename = imgui_ini_path;

    ImGui_ImplGlfw_InitForVulkan(window, false);

    FilePath imgui_vert_path = "shaders/imgui.vert.spv"_distributed;
    FilePath imgui_frag_path = "shaders/imgui.frag.spv"_distributed;

    imgui_data = ImGui_ImplVuk_Init(*global_allocator, compiler, {
        imgui_vert_path.abs_string(), get_contents_uint32(imgui_vert_path),
        imgui_frag_path.abs_string(), get_contents_uint32(imgui_frag_path)
    });

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::GetIO().ConfigDockingWithShift = true;

    {
        vuk::PipelineBaseCreateInfo pci;
        pci.add_glsl(get_contents("shaders/postprocess.comp"_distributed), "shaders/postprocess.comp"_distributed.abs_string());
        runtime->runtime->create_named_pipeline("postprocess", pci);
    }
    {
        vuk::PipelineBaseCreateInfo pci;
        pci.add_glsl(get_contents("shaders/blur.comp"_distributed), "shaders/blur.comp"_distributed.abs_string());
        runtime->runtime->create_named_pipeline("blur", pci);
    }
    {
        vuk::PipelineBaseCreateInfo pci;
        pci.set_compile_options(
            {
                .optimization_level = vuk::ShaderCompileOptions::OptimizationLevel::O0
            }
        );
        pci.add_glsl(get_contents("shaders/diffusion.comp"_distributed), "shaders/diffusion.comp"_distributed.abs_string());
        runtime->runtime->create_named_pipeline("diffusion", pci);
    }
    {
        vuk::PipelineBaseCreateInfo pci;
        pci.add_glsl(get_contents("shaders/standard_3d.vert"_distributed), "shaders/standard_3d.vert"_distributed.abs_string());
        pci.add_glsl(get_contents("shaders/textured_3d.frag"_distributed), "shaders/textured_3d.frag"_distributed.abs_string());
        runtime->runtime->create_named_pipeline("textured_model", pci);
    }
    {
        vuk::PipelineBaseCreateInfo pci;
        pci.add_glsl(get_contents("shaders/standard_3d.vert"_distributed), "shaders/standard_3d.vert"_distributed.abs_string());
        pci.add_glsl(get_contents("shaders/desert_rocks.frag"_distributed), "shaders/desert_rocks.frag"_distributed.abs_string());
        runtime->runtime->create_named_pipeline("desert_rocks", pci);
    }
    {
        vuk::PipelineBaseCreateInfo pci;
        pci.add_glsl(get_contents("shaders/standard_3d.vert"_distributed), "shaders/standard_3d.vert"_distributed.abs_string());
        pci.add_glsl(get_contents("shaders/point_depth.frag"_distributed), "shaders/point_depth.frag"_distributed.abs_string());
        runtime->runtime->create_named_pipeline("point_depth", pci);
    }
    {
        vuk::PipelineBaseCreateInfo pci;
        pci.add_glsl(get_contents("shaders/directional_depth.vert"_distributed), "shaders/directional_depth.vert"_distributed.abs_string());
        pci.add_glsl(get_contents("shaders/directional_depth.frag"_distributed), "shaders/directional_depth.frag"_distributed.abs_string());
        runtime->runtime->create_named_pipeline("directional_depth", pci);
    }
    {
        vuk::PipelineBaseCreateInfo pci;
        pci.add_glsl(get_contents("shaders/infinite_plane.vert"_distributed), "shaders/infinite_plane.vert"_distributed.abs_string());
        pci.add_glsl(get_contents("shaders/grid.frag"_distributed), "shaders/grid.frag"_distributed.abs_string());
        runtime->runtime->create_named_pipeline("grid_3d", pci);
    }

    get_gpu_asset_cache().upload_defaults();

    {
        // OPTIMIZATION: can thread
        for (auto scene : scenes) {
            scene->setup(*global_allocator);
        }
    }

    wait_for_futures();

    stage = RenderStage_Inactive;

    Input::add_callback<ResizeCallback>(InputCallbackInfo<ResizeCallback>{
        .callback = [](ResizeCallbackArgs args)-> bool {
            get_renderer().resize({args.x, args.y});
            return false;
        },
        .priority = 0,
        .name = "framebuffer_resize"
    });
}

void Renderer::update() {
    ZoneScoped;
    while (suspend) {
        glfwWaitEvents();
    }
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    frame_timer.update();

    auto& frame_resource = runtime->superframe_resource->get_next_frame();
    runtime->next_frame();
    frame_allocator.emplace(frame_resource);

    for (auto scene : scenes) {
        scene->update();
    }
}

void Renderer::render() {
    ZoneScoped;
    assert_else(stage == RenderStage_Inactive)
        return;

    wait_for_futures();

    stage = RenderStage_BuildingRG;

    if (do_extra_passes) {
        for (auto& pass : extra_passes) {
            pass();
        }
    }
    wait_for_futures();

    for (auto scene : scenes) {
        ZoneScoped;
        scene->pre_render();
        scene->render();
    }
}

void Renderer::present() {
    ImGui::Render();

	auto imported_swapchain = acquire_swapchain(*runtime->swapchain);
	// acquire an image on the swapchain
	auto swapchain_image = acquire_next_image("swp_img", std::move(imported_swapchain));
	// clear the swapchain image
	vuk::Value<vuk::ImageAttachment> cleared_image_to_render_into = vuk::clear_image(std::move(swapchain_image), vuk::ClearColor{0.1f, 0.05f, 0.1f, 1.0f});

    auto imgui = ImGui_ImplVuk_Render(*frame_allocator, cleared_image_to_render_into, imgui_data, compiler);
    
    stage    = RenderStage_Presenting;
	auto entire_thing = enqueue_presentation(std::move(imgui));

	vuk::ProfilingCallbacks cbs;
#ifdef TRACY_ENABLE
	// set up some profiling callbacks for our Tracy integration
	cbs = make_Tracy_callbacks(*tracy_context);
#endif

	entire_thing.submit(frame_allocator.value(), compiler, {.callbacks = cbs});

    for (auto scene : scenes) {
        for (auto it = scene->renderables.begin(); it != scene->renderables.end();) {
            if (it->frame_allocated)
                it = scene->renderables.erase(it);
            else
                ++it;
        }
        for (auto it = scene->widget_renderables.begin(); it != scene->widget_renderables.end();) {
            if (it->frame_allocated)
                it = scene->widget_renderables.erase(it);
            else
                ++it;
        }

        for (auto it = scene->ui_renderables.begin(); it != scene->ui_renderables.end();) {
            if (it->frame_allocated)
                it = scene->ui_renderables.erase(it);
            else
                ++it;
        }
    }

    get_gpu_asset_cache().clear_frame_allocated_assets();
    frame_allocator.reset();

    stage = RenderStage_Inactive;
}

void Renderer::cleanup() {
    runtime->runtime->wait_idle();
    for (auto scene : scenes) {
        scene->cleanup(*global_allocator);
    }
    get_gpu_asset_cache().clear();
}

Renderer::~Renderer() {
	imgui_data.font_image.reset();
	imgui_data.font_image_view.reset();
#ifdef TRACY_ENABLE
	tracy_context.reset();
#endif
	runtime.reset();
	glfwDestroyWindow(window);
	glfwTerminate();
}

void Renderer::wait_for_futures() {
    ZoneScoped;
	vuk::wait_for_values_explicit(*runtime->superframe_allocator, compiler, futures);
	futures.clear();
}


void Renderer::resize(v2i new_size) {
    if (new_size == v2i(0,0)) {
        suspend = true;
        return;
    }
    suspend = false;
    runtime->update_swapchain();
}

void Renderer::debug_window(bool* p_open) {
    ZoneScoped;
    if (ImGui::Begin("Renderer", p_open)) {
        ImGui::Text(fmt_("Window Size: {}", window_size).c_str());

        ImGui::Checkbox("Extra Passes", &do_extra_passes);

        frame_timer.inspect();
    }
    ImGui::End();
}

void FrameTimer::update() {
    current_frame++;
    int32 last_index = ptr;
    float last_time  = frame_times[ptr];
    ptr              = (ptr + 1) % 200;
    frame_times[ptr] = Input::time;

    if (filled > 1)
        delta_times[last_index] = frame_times[ptr] - last_time;
    filled = math::min(filled + 1, 200);
}

void FrameTimer::inspect() {
    float average = 0.0f;
    for (int n = 0; n < filled; n++)
        average += delta_times[n];
    average /= (float) filled;
    string overlay = fmt_("FPS: {:.1f}", 1.0f / average);
    ImGui::PlotLines("DT", delta_times.data(), filled, ptr, overlay.c_str(), 0.0f, 0.1f, ImVec2(0, 80.0f));
}

}
