
#include <vuk/vsl/Core.hpp>

#include "general/math/math.hpp"
#include "general/file/file_path.hpp"
#include "renderer/renderer.hpp"
#include "renderer/render_scene.hpp"
#include "renderer/runtime.hpp"
#include "editor/editor_scene.hpp"
#include "game/scene.hpp"

namespace spellbook {

struct DiffusionTest : EditorScene {
    vuk::Unique<vuk::Image> image_diffusion;
    vuk::Unique<vuk::ImageView> image_view_diffusion;
    vuk::ImageAttachment texture_diffusion;
    vuk::Value<vuk::ImageAttachment> texture_fut_diffusion;

    v2i size = {512, 512};

    float seed_pct = 0.05f;

    float da = 1.0f;
    float db = 0.5f;
    float f = 0.055f;
    float k = 0.062f;
    float dt = 1.0f;

    float step_min = 0.30f;
    float step_max = 0.31f;

    void setup() override;
    void update() override;
    void window(bool* p_open) override;
    void shutdown() override;

    void setup_initial_texture();
};
//ADD_EDITOR_SCENE(DiffusionTest);

void DiffusionTest::setup_initial_texture() {
    static bool init = false;
    if (!init) {
        {
		    vuk::PipelineBaseCreateInfo pbci;
		    pbci.add_glsl(get_contents("shaders/diffusion.comp"_distributed), "shaders/diffusion.comp");
		    get_renderer().runtime->runtime->create_named_pipeline("diffusion", pbci);
        }
        {
		    vuk::PipelineBaseCreateInfo pbci;
		    pbci.add_glsl(get_contents("shaders/diffusion_display.comp"_distributed), "shaders/diffusion_display.comp");
		    get_renderer().runtime->runtime->create_named_pipeline("diffusion_display", pbci);
        }

        init = true;
	}

    vector<float> pixels;
    pixels.resize(size.x * size.y * 2, 0.0f);
    v2i middle = size / 2;

    for (int x = 0; x < size.x; x++) {
        for (int y = 0; y < size.y; y++) {
            bool seeded = math::random_float() <= seed_pct;
            pixels[(size.x * y + x) * 2 + 0] = seeded ? 0.0f : 1.0f;
            pixels[(size.x * y + x) * 2 + 1] = seeded ? 1.0f : 0.0f;
        }
    }

	texture_diffusion = vuk::ImageAttachment::from_preset(vuk::ImageAttachment::Preset::eGeneric2D, vuk::Format::eR32G32Sfloat, vuk::Extent3D{ (unsigned)size.x, (unsigned)size.y, 1u }, vuk::Samples::e1);
	texture_diffusion.level_count = 1;
	auto [image, view, future] = vuk::create_image_and_view_with_data(*get_renderer().global_allocator, vuk::DomainFlagBits::eTransferOnTransfer, texture_diffusion, pixels.data());
	image_diffusion = std::move(image);
	image_view_diffusion = std::move(view);
    texture_fut_diffusion = future;
    get_renderer().enqueue_setup(std::move(future));
}

void DiffusionTest::setup() {
    get_renderer().do_extra_passes = true;
    setup_initial_texture();

    get_renderer().extra_passes.push_back([this]() {
        auto intermediate = vuk::declare_ia(
		    "diffusion_intermediate",
		    {
                .usage = vuk::ImageUsageFlagBits::eStorage | vuk::ImageUsageFlagBits::eTransferSrc | vuk::ImageUsageFlagBits::eSampled,
                .extent = { (unsigned)size.x, (unsigned)size.y, 1u },
                .format = vuk::Format::eR32G32Sfloat,
                .sample_count = vuk::Samples::e1,
                .level_count = 1,
                .layer_count = 1
            }
        );

        auto diffusion_display_image = vuk::declare_ia(
		    "diffusion_display_image",
		    {
                .usage = vuk::ImageUsageFlagBits::eStorage | vuk::ImageUsageFlagBits::eSampled,
                .extent = { (unsigned)size.x, (unsigned)size.y, 1u },
                .format = vuk::Format::eR16G16B16A16Sfloat,
                .sample_count = vuk::Samples::e1,
                .level_count = 1,
                .layer_count = 1
            }
        );

		// this pass executes outside of a renderpass
		// we declare a buffer dependency and dispatch a compute shader
		auto diffusion_pass = vuk::make_pass(
		    "diffusion",
		    [this](vuk::CommandBuffer& command_buffer, VUK_IA(vuk::eComputeRead) primary, VUK_IA(vuk::eComputeWrite) intermediate) {
			    command_buffer.bind_image(0, 0, primary);
			    command_buffer.bind_image(0, 1, intermediate);
			    command_buffer.bind_compute_pipeline("diffusion");

                command_buffer.push_constants(vuk::ShaderStageFlagBits::eCompute, 0 * sizeof(float), da); 
                command_buffer.push_constants(vuk::ShaderStageFlagBits::eCompute, 1 * sizeof(float), db); 
                command_buffer.push_constants(vuk::ShaderStageFlagBits::eCompute, 2 * sizeof(float), f); 
                command_buffer.push_constants(vuk::ShaderStageFlagBits::eCompute, 3 * sizeof(float), k); 
                command_buffer.push_constants(vuk::ShaderStageFlagBits::eCompute, 4 * sizeof(float), dt); 

                command_buffer.dispatch_invocations(size.x, size.y);

			    return intermediate;
		    },
		    vuk::DomainFlagBits::eGraphicsQueue
        );
        intermediate = diffusion_pass(texture_fut_diffusion, intermediate);

		auto blit_pass = vuk::make_pass(
		    "blit",
		    [this](vuk::CommandBuffer& command_buffer, VUK_IA(vuk::eTransferRead) src, VUK_IA(vuk::eTransferWrite) dst) {
                vuk::ImageBlit blit;
                blit.srcSubresource.aspectMask = vuk::ImageAspectFlagBits::eColor;
                blit.srcSubresource.baseArrayLayer = 0;
                blit.srcSubresource.layerCount = 1;
                blit.srcSubresource.mipLevel = 0;
                blit.srcOffsets[0] = {0,0,0};
                blit.srcOffsets[1] = {size.x,size.y,1};

                blit.dstSubresource = blit.srcSubresource;
                blit.dstOffsets[0] = {0,0,0};
                blit.dstOffsets[1] = {size.x,size.y,1};
                command_buffer.blit_image(src, dst, blit, vuk::Filter::eNearest);
                return dst;
		    },
		    vuk::DomainFlagBits::eGraphicsQueue
        );

		auto display_pass = vuk::make_pass(
		    "diffusion_display",
		    [this](vuk::CommandBuffer& command_buffer, VUK_IA(vuk::eComputeRead) src, VUK_IA(vuk::eComputeWrite) dst) {
			    command_buffer.bind_image(0, 0, src);
			    command_buffer.bind_image(0, 1, dst);
			    command_buffer.bind_compute_pipeline("diffusion_display");

                command_buffer.push_constants(vuk::ShaderStageFlagBits::eCompute, 0 * sizeof(float), step_min); 
                command_buffer.push_constants(vuk::ShaderStageFlagBits::eCompute, 1 * sizeof(float), step_max); 

                command_buffer.dispatch_invocations(size.x, size.y);

			    return dst;
		    },
		    vuk::DomainFlagBits::eGraphicsQueue
        );

        texture_fut_diffusion = blit_pass(intermediate, texture_fut_diffusion);

        texture_fut_diffusion.submit(*get_renderer().frame_allocator, get_renderer().compiler);

        diffusion_display_image = display_pass(intermediate, diffusion_display_image);

        if (ImGui::Begin("Diffusion")) {
            ImGui::Image(get_renderer().imgui_data.add_image(std::move(diffusion_display_image)), ImVec2(size));
        }
        ImGui::End();
    });
}

void DiffusionTest::update() {
}

void DiffusionTest::window(bool* p_open) {
    if (ImGui::Begin("Diffusion Window", p_open)) {
        ImGui::DragFloat("da", &da, 0.01f);
        ImGui::DragFloat("db", &db, 0.01f);
        ImGui::DragFloat("f", &f, 0.01f);
        ImGui::DragFloat("k", &k, 0.01f);
        ImGui::DragFloat("dt", &dt, 0.01f);
        ImGui::Separator();
        ImGui::DragFloat("seed_pct", &seed_pct, 0.01f);

        if (ImGui::Button("Reseed"))
            setup_initial_texture();    

        ImGui::Separator();
        ImGui::DragFloat("step_min", &step_min, 0.01f);
        ImGui::DragFloat("step_max", &step_max, 0.01f);
    }
    ImGui::End();
}

void DiffusionTest::shutdown() {
	image_diffusion.reset();
	image_view_diffusion.reset();
}

}