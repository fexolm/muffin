#pragma once

#include <memory>
#include <vulkan/vulkan.h>

#include "VulkanDevice.h"

class VulkanDescriptorPool
{
public:
	VulkanDescriptorPool(VulkanDeviceRef device, VkDescriptorPool descriptorPool);

	virtual ~VulkanDescriptorPool();

	VkDescriptorPool Handle();

private:
	VulkanDeviceRef device;
	VkDescriptorPool descriptorPool;
};

using VulkanDescriptorPoolRef = std::shared_ptr<VulkanDescriptorPool>;