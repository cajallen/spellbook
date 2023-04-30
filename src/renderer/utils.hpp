#pragma once

#include "general/vector.hpp"

#include <VkBootstrap.h>
#include <vuk/Swapchain.hpp>
#include <vuk/Types.hpp>

namespace spellbook {

inline vuk::Swapchain make_swapchain(vkb::Device vkbdevice, vuk::Swapchain* old = nullptr) {
	vkb::SwapchainBuilder swb(vkbdevice);
	swb.set_desired_format(vuk::SurfaceFormatKHR {vuk::Format::eB8G8R8A8Srgb, vuk::ColorSpaceKHR::eSrgbNonlinear});
	swb.add_fallback_format(vuk::SurfaceFormatKHR {vuk::Format::eB8G8R8A8Srgb, vuk::ColorSpaceKHR::eSrgbNonlinear});
	swb.set_desired_present_mode((VkPresentModeKHR) vuk::PresentModeKHR::eImmediate);
	swb.set_image_usage_flags(VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    if (old)
        swb.set_old_swapchain(old->swapchain);
	auto vkswapchain = swb.build();

	vuk::Swapchain sw{};
	auto		   images = vkswapchain->get_images();
	auto		   views  = vkswapchain->get_image_views();

	for (auto& i : *images) {
		sw.images.push_back(vuk::Image{i, nullptr});
	}
	for (auto& i : *views) {
		sw.image_views.emplace_back();
		sw.image_views.back().payload = i;
	}
	sw.extent	 = vuk::Extent2D {vkswapchain->extent.width, vkswapchain->extent.height};
	sw.format	 = vuk::Format(vkswapchain->image_format);
	sw.surface	 = vkbdevice.surface;
	sw.swapchain = vkswapchain->swapchain;
	return sw;
}

}