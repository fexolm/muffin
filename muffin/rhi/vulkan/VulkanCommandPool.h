#pragma once

#include "VulkanDevice.h"

#include <vulkan/vulkan.h>

class VulkanCommandPool
{
public:
	VulkanCommandPool(VulkanDeviceRef device, VkCommandPool commandPool);

	const VkCommandPool& CommandPool();

	~VulkanCommandPool();

private:
	VulkanDeviceRef device;
	VkCommandPool commandPool;
};

using VulkanCommandPoolRef = std::shared_ptr<VulkanCommandPool>;