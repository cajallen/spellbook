#pragma once

#include <vuk/Types.hpp>
#include <vuk/Image.hpp>
#include <vuk/SampledImage.hpp>
#include <plf_colony.h>
#include <imgui.h>

namespace spellbook {

struct ImGuiData {
    vuk::Texture                       font_texture;
    vuk::SamplerCreateInfo             font_sci;
    std::unique_ptr<vuk::SampledImage> font_si;
};
ImGuiData   ImGui_ImplVuk_Init(vuk::Allocator& allocator);
vuk::Future ImGui_ImplVuk_Render(vuk::Allocator& allocator, vuk::Future target, ImGuiData& data, ImDrawData* draw_data,
    const plf::colony<vuk::SampledImage>& sampled_images);

}