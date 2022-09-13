#pragma once

#include "VulkanInstance.h"

#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

class VulkanDevice
{
public:
	VulkanDevice(VulkanInstanceRef instance,
		const std::vector<const char*>& deviceExtensions,
		VkSurfaceKHR surface);

	virtual ~VulkanDevice();

	const VkDevice& Device();

	const VkPhysicalDevice& PhysicalDevice();

	uint32_t GraphicsFamily();

	uint32_t PresentFamily();

	const VkQueue& GraphicsQueue();

	const VkQueue& PresentQueue();

private:
	VulkanInstanceRef instance;
	VkDevice device;
	VkPhysicalDevice physicalDevice;
	uint32_t graphicsFamilyIdx;
	uint32_t presentFamilyIdx;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
};

using VulkanDeviceRef = std::shared_ptr<VulkanDevice>;