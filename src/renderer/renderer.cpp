﻿#include "renderer.hpp"

#include <vuk/src/RenderGraphUtil.hpp>
#include <vuk/Partials.hpp>
#include <vuk/Allocator.hpp>
#include <backends/imgui_impl_glfw.h>
#include <tracy/Tracy.hpp>
#include <stb_image.h>

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
#include "renderer/samplers.hpp"
#include "renderer/utils.hpp"
#include "renderer/gpu_asset_cache.hpp"
#include "renderer/font_manager.hpp"

namespace spellbook {

Renderer& get_renderer() {
    static Renderer renderer;
    return renderer;
}

Renderer::Renderer() : imgui_data() {
    vkb::InstanceBuilder builder;
    builder
        .request_validation_layers(false)
        .set_debug_callback([](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT                           messageType,
            const VkDebugUtilsMessengerCallbackDataEXT*               pCallbackData,
            void*                                                     pUserData) -> VkBool32 {
                auto ms = vkb::to_string_message_severity(messageSeverity);
                auto mt = vkb::to_string_message_type(messageType);
                log_error(fmt_("[{}: {}]\n{}\n", ms, mt, pCallbackData->pMessage), "renderer");
                __debugbreak();
                return VK_FALSE;
            })
        .set_app_name("saurian_sorcery")
        .set_engine_name("spellbook")
        .require_api_version(1, 2, 0)
        .set_app_version(0, 1, 0);
    auto inst_ret = builder.build();
    assert_else(inst_ret.has_value())
        return;
    vkbinstance                          = inst_ret.value();
    auto                        instance = vkbinstance.instance;
    vkb::PhysicalDeviceSelector selector{vkbinstance};
    VkPhysicalDeviceFeatures    vkfeatures{
        .independentBlend = VK_TRUE,
        .samplerAnisotropy = VK_TRUE
    };
    window  = create_window_glfw("Spellbook", window_size, true);
    surface = create_surface_glfw(vkbinstance.instance, window);

    GLFWimage image;

    string icon_path = ("icon.png"_distributed).abs_string();
    image.pixels = stbi_load(icon_path.c_str(), &image.width, &image.height, nullptr, 4);
    if (image.pixels != nullptr) {
        glfwSetWindowIcon(window, 1, &image);
        stbi_image_free(image.pixels);
    }

    selector.set_surface(surface)
            .set_minimum_version(1, 0)
            .set_required_features(vkfeatures)
            .add_required_extension(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME)
            .add_required_extension(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME);
    auto phys_ret = selector.select();
    assert_else(phys_ret.has_value());
    vkb::PhysicalDevice vkbphysical_device = phys_ret.value();
    physical_device                        = vkbphysical_device.physical_device;

    vkb::DeviceBuilder               device_builder{vkbphysical_device};
    VkPhysicalDeviceVulkan12Features vk12features{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    vk12features.timelineSemaphore                         = true;
    vk12features.descriptorBindingPartiallyBound           = true;
    vk12features.descriptorBindingUpdateUnusedWhilePending = true;
    vk12features.shaderSampledImageArrayNonUniformIndexing = true;
    vk12features.runtimeDescriptorArray                    = true;
    vk12features.descriptorBindingVariableDescriptorCount  = true;
    vk12features.hostQueryReset                            = true;
    vk12features.shaderOutputLayer                         = true;
    vk12features.bufferDeviceAddress                       = true; // vuk requirement
    vk12features.vulkanMemoryModel                         = true; // general performance improvement
    vk12features.vulkanMemoryModelDeviceScope              = true; // general performance improvement
    VkPhysicalDeviceVulkan11Features vk11features{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    vk11features.shaderDrawParameters = true;
    VkPhysicalDeviceSynchronization2FeaturesKHR sync_feat{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR,
        .synchronization2 = true
    };

    device_builder = device_builder.add_pNext(&vk12features).add_pNext(&vk11features).add_pNext(&sync_feat);
    auto dev_ret   = device_builder.build();
    assert_else(dev_ret.has_value());
    vkbdevice                        = dev_ret.value();
    graphics_queue                   = vkbdevice.get_queue(vkb::QueueType::graphics).value();
    auto graphics_queue_family_index = vkbdevice.get_queue_index(vkb::QueueType::graphics).value();
    transfer_queue                   = vkbdevice.get_queue(vkb::QueueType::transfer).value();
    auto transfer_queue_family_index = vkbdevice.get_queue_index(vkb::QueueType::transfer).value();
    device                           = vkbdevice.device;

    vuk::ContextCreateParameters::FunctionPointers fps;
    fps.vkGetInstanceProcAddr = vkbinstance.fp_vkGetInstanceProcAddr;
    fps.vkGetDeviceProcAddr = vkbinstance.fp_vkGetDeviceProcAddr;
    
    context.emplace(vuk::ContextCreateParameters{instance,
         device,
         physical_device,
         graphics_queue,
         graphics_queue_family_index,
         VK_NULL_HANDLE,
         VK_QUEUE_FAMILY_IGNORED,
         transfer_queue,
         transfer_queue_family_index,
        fps});
    const unsigned num_inflight_frames = 3;
    super_frame_resource.emplace(*context, num_inflight_frames);
    global_allocator.emplace(*super_frame_resource);
    swapchain = context->add_swapchain(make_swapchain(vkbdevice));

    present_ready = vuk::Unique<std::array<VkSemaphore, 3>>(*global_allocator);
    render_complete = vuk::Unique<std::array<VkSemaphore, 3>>(*global_allocator);

    global_allocator->allocate_semaphores(*present_ready);
    global_allocator->allocate_semaphores(*render_complete);
}

void Renderer::enqueue_setup(vuk::Future&& fut) {
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
    imgui_ini_path = new char[256];
    strcpy(imgui_ini_path, ("imgui.ini"_config).abs_string().c_str());
    ImGui::GetIO().IniFilename = imgui_ini_path;

    ImGui_ImplGlfw_InitForVulkan(window, false);

    FilePath imgui_vert_path = "shaders/imgui.vert.spv"_distributed;
    FilePath imgui_frag_path = "shaders/imgui.frag.spv"_distributed;
    ImGuiShaderInfo imgui_shaders = {
        imgui_vert_path.abs_string(), get_contents_uint32(imgui_vert_path),
        imgui_frag_path.abs_string(), get_contents_uint32(imgui_frag_path)
    };

    imgui_data = ImGui_ImplVuk_Init(*global_allocator, compiler, imgui_shaders);

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ImGui::GetIO().ConfigDockingWithShift = true;

    {
        vuk::PipelineBaseCreateInfo pci;
        pci.add_glsl(get_contents("shaders/postprocess.comp"_distributed), "shaders/postprocess.comp"_distributed.abs_string());
        context->create_named_pipeline("postprocess", pci);
    }
    {
        vuk::PipelineBaseCreateInfo pci;
        pci.add_glsl(get_contents("shaders/blur.comp"_distributed), "shaders/blur.comp"_distributed.abs_string());
        context->create_named_pipeline("blur", pci);
    }
    {
        vuk::PipelineBaseCreateInfo pci;
        pci.add_glsl(get_contents("shaders/standard_3d.vert"_distributed), "shaders/standard_3d.vert"_distributed.abs_string());
        pci.add_glsl(get_contents("shaders/textured_3d.frag"_distributed), "shaders/textured_3d.frag"_distributed.abs_string());
        context->create_named_pipeline("textured_model", pci);
    }
    {
        vuk::PipelineBaseCreateInfo pci;
        pci.add_glsl(get_contents("shaders/standard_3d.vert"_distributed), "shaders/standard_3d.vert"_distributed.abs_string());
        pci.add_glsl(get_contents("shaders/desert_rocks.frag"_distributed), "shaders/desert_rocks.frag"_distributed.abs_string());
        context->create_named_pipeline("desert_rocks", pci);
    }
    {
        vuk::PipelineBaseCreateInfo pci;
        pci.add_glsl(get_contents("shaders/standard_3d.vert"_distributed), "shaders/standard_3d.vert"_distributed.abs_string());
        pci.add_glsl(get_contents("shaders/point_depth.frag"_distributed), "shaders/point_depth.frag"_distributed.abs_string());
        context->create_named_pipeline("point_depth", pci);
    }
    {
        vuk::PipelineBaseCreateInfo pci;
        pci.add_glsl(get_contents("shaders/directional_depth.vert"_distributed), "shaders/directional_depth.vert"_distributed.abs_string());
        pci.add_glsl(get_contents("shaders/directional_depth.frag"_distributed), "shaders/directional_depth.frag"_distributed.abs_string());
        context->create_named_pipeline("directional_depth", pci);
    }
    {
        vuk::PipelineBaseCreateInfo pci;
        pci.add_glsl(get_contents("shaders/infinite_plane.vert"_distributed), "shaders/infinite_plane.vert"_distributed.abs_string());
        pci.add_glsl(get_contents("shaders/grid.frag"_distributed), "shaders/grid.frag"_distributed.abs_string());
        context->create_named_pipeline("grid_3d", pci);
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

    auto& frame_resource = super_frame_resource->get_next_frame();
    context->next_frame();
    frame_allocator.emplace(frame_resource);

    for (auto scene : scenes) {
        scene->update();
    }
}

vuk::SingleSwapchainRenderBundle bundle;
void Renderer::render() {
    ZoneScoped;
    assert_else(stage == RenderStage_Inactive)
        return;

    wait_for_futures();

    stage = RenderStage_BuildingRG;
    
    shared_ptr<vuk::RenderGraph> rg = std::make_shared<vuk::RenderGraph>("renderer");
    std::vector resources{"SWAPCHAIN+"_image >> vuk::eColorWrite >> "SWAPCHAIN++"};
    for (auto scene : scenes) {
        ZoneScoped;
        scene->pre_render();

        std::shared_ptr<vuk::RenderGraph> rgx = std::make_shared<vuk::RenderGraph>(vuk::Name(scene->name));
        rgx->attach_image("input_uncleared", vuk::ImageAttachment::from_texture(scene->render_target));
        rgx->clear_image("input_uncleared", vuk::Name(scene->name + "_input"), vuk::ClearColor{0.1f, 0.1f, 0.1f, 1.0f});
        auto scene_fut     = scene->render(*frame_allocator, vuk::Future{rgx, vuk::Name(scene->name + "_input")});
        rg->attach_in(vuk::Name(scene->name + "_final"), std::move(scene_fut));

        resources.emplace_back(vuk::Resource {vuk::Name(scene->name + "_final"), vuk::Resource::Type::eImage, vuk::Access::eFragmentSampled});
    }

    ImGui::Render();
    // NOTE: When we render in 3D, we're using reverse depth. We have no need for that here because we don't have depth precision issues
    rg->clear_image("SWAPCHAIN", "SWAPCHAIN+", vuk::ClearColor{0.1f, 0.05f, 0.1f, 1.0f});
    rg->attach_swapchain("SWAPCHAIN", swapchain);
    
    rg->add_pass({.name = "force_transition", .resources = std::move(resources)});

    auto fut = ImGui_ImplVuk_Render(*frame_allocator, vuk::Future{rg, "SWAPCHAIN++"}, imgui_data, ImGui::GetDrawData(), imgui_images, compiler);
    
    auto rg_p = std::make_shared<vuk::RenderGraph>("presenter");
    rg_p->attach_in("_src", std::move(fut));
    // we tell the rendergraph that _src will be used for presenting after the rendergraph
    rg_p->release_for_present("_src");
    
    stage    = RenderStage_Presenting;
    auto erg = *compiler.link(std::span{ &rg_p, 1 }, {});
    bundle = *acquire_one(*context, swapchain, (*present_ready)[context->get_frame_count() % 3], (*render_complete)[context->get_frame_count() % 3]);
    auto result = *execute_submit(*frame_allocator, std::move(erg), std::move(bundle));
    present_to_one(*context, std::move(result));
    imgui_images.clear();

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
                it = scene->ui_renderables.remove(it);
            else
                ++it;
        }
    }

    get_gpu_asset_cache().clear_frame_allocated_assets();
    frame_allocator.reset();

    stage = RenderStage_Inactive;
}

void Renderer::cleanup() {
    context->wait_idle();
    for (auto scene : scenes) {
        scene->cleanup(*global_allocator);
    }
    get_gpu_asset_cache().clear();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    delete imgui_ini_path;
}

Renderer::~Renderer() {
    present_ready.reset();
    render_complete.reset();
    imgui_data.font_texture.view.reset();
    imgui_data.font_texture.image.reset();
    super_frame_resource.reset();
    context.reset();
    vkDestroySurfaceKHR(vkbinstance.instance, surface, nullptr);
    destroy_window_glfw(window);
    vkb::destroy_device(vkbdevice);
    vkb::destroy_instance(vkbinstance);
}

void Renderer::wait_for_futures() {
    ZoneScoped;
    vuk::wait_for_futures_explicit(*global_allocator, compiler, futures);
    futures.clear();
}


void Renderer::resize(v2i new_size) {
    if (new_size == v2i(0,0)) {
        suspend = true;
        return;
    }
    suspend = false;
    context->wait_idle();
    global_allocator->deallocate(swapchain->image_views);
    global_allocator->deallocate({&swapchain->swapchain, 1});
    context->remove_swapchain(swapchain);
    auto new_swapchain = context->add_swapchain(make_swapchain(vkbdevice, swapchain));
    for (auto& iv : swapchain->image_views) {
        context->set_name(iv.payload, "Swapchain ImageView");
    }
    swapchain = new_swapchain;
}

void Renderer::debug_window(bool* p_open) {
    ZoneScoped;
    if (ImGui::Begin("Renderer", p_open)) {
        ImGui::Text(fmt_("Window Size: {}", window_size).c_str());

        frame_timer.inspect();
    }
    ImGui::End();
}

void FrameTimer::update() {
    int last_index   = ptr;
    float last_time    = frame_times[ptr];
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
