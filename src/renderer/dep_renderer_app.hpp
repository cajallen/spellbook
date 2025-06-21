#include <VkBootstrap.h>

#include <vuk/runtime/vk/DeviceFrameResource.hpp>
#include <vuk/runtime/vk/VkRuntime.hpp>

namespace spellbook {

struct AppRuntime {
	vkb::Instance vkbinstance;
	vkb::Device vkbdevice;

	uint32_t vk_api_major_version;
	uint32_t vk_api_minor_version;
	uint32_t vk_device_version;
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	std::optional<vuk::Runtime> runtime;
	std::optional<vuk::DeviceSuperFrameResource> superframe_resource;
	std::optional<vuk::Allocator> superframe_allocator;
	std::optional<vuk::Swapchain> swapchain = {};

	void update_swapchain();
	void wait_idle();
	void next_frame();

	~AppRuntime();
};

struct PhysicalDevice : vkb::PhysicalDevice {
	vkb::Instance instance;
	uint32_t rq_major_version;
	uint32_t rq_minor_version;
};

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

	/// @brief set all recommended features:
	DeviceBuilder& set_recommended_features();

	/// @brief Build the Vulkan device only, use this if you want to create a vuk::Runtime yourself
	vkb::Result<vkb::Device> build_device_only();
	/// @brief Build the Vulkan device, vuk::Runtime and AppRuntime
	std::unique_ptr<AppRuntime> build_app(bool with_swapchain = true, unsigned num_inflight_frames = 3);

private:
	PhysicalDevice physical_device;
};

vkb::InstanceBuilder make_instance_builder(uint32_t vulkan_major_version, uint32_t vulkan_minor_version, bool with_default_callback);

PhysicalDevice select_physical_device(vkb::Instance instance, VkSurfaceKHR surface);

DeviceBuilder make_device_builder(PhysicalDevice physical_device);

vuk::Swapchain make_swapchain(vuk::Allocator allocator, vkb::Device vkbdevice, VkSurfaceKHR surface, std::optional<vuk::Swapchain> old_swapchain);

}