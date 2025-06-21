#pragma once

#include <optional>

#include <VkBootstrap.h>
#include <vuk/runtime/vk/DeviceFrameResource.hpp>
#include <vuk/runtime/vk/VkRuntime.hpp>

#include "general/memory.hpp"

using std::optional;

namespace spellbook {

/// @brief Wrapper around vuk::Runtime that manages Vulkan instance, device, swapchain and superframe resources
struct Runtime {
	vkb::Instance vkbinstance;
	vkb::Device vkbdevice;

	uint32 vk_api_major_version;
	uint32 vk_api_minor_version;
	uint32 vk_device_version;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	optional<vuk::Runtime> runtime;
	optional<vuk::DeviceSuperFrameResource> superframe_resource;
	optional<vuk::Allocator> superframe_allocator;
	optional<vuk::Swapchain> swapchain = {};

	void update_swapchain();
	void wait_idle();
	void next_frame();

	~Runtime();
};

/// @brief Wrapper around vk-bootstrap's PhysicalDevice
struct PhysicalDevice : vkb::PhysicalDevice {
	vkb::Instance instance;
	uint32 rq_major_version;
	uint32 rq_minor_version;
};

/// @brief Wrapper around vk-bootstrap's DeviceBuilder
struct DeviceBuilder : vkb::DeviceBuilder {
	explicit DeviceBuilder(PhysicalDevice physical_device);

	VkPhysicalDeviceFeatures2 vk10features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR };
	VkPhysicalDeviceVulkan11Features vk11features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
	VkPhysicalDeviceVulkan12Features vk12features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
#ifdef VK_VERSION_1_3
	VkPhysicalDeviceVulkan13Features vk13features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
#endif
#ifdef VK_VERSION_1_4
	VkPhysicalDeviceVulkan14Features vk14features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES };
#endif

	DeviceBuilder& set_recommended_features();

	vkb::Result<vkb::Device> build_device_only();
	unique_ptr<Runtime> build_app(bool with_swapchain = true, uint32 num_inflight_frames = 3);

private:
	PhysicalDevice physical_device;
};

vkb::InstanceBuilder make_instance_builder(uint32 vulkan_major_version, uint32 vulkan_minor_version, bool with_default_callback);

PhysicalDevice select_physical_device(vkb::Instance instance, VkSurfaceKHR surface);

DeviceBuilder make_device_builder(PhysicalDevice physical_device);

vuk::Swapchain make_swapchain(vuk::Allocator allocator, vkb::Device vkbdevice, VkSurfaceKHR surface, optional<vuk::Swapchain> old_swapchain);

}
