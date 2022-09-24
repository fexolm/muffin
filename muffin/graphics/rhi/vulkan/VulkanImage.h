#pragma once

#include "VulkanDevice.h"
#include "muffin/graphics/rhi/RHI.h"

#include <vulkan/vulkan.h>

struct VulkanImage : RHITexture
{
	VulkanDeviceRef device;
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;

	VulkanImage(VulkanDeviceRef device, VkImage img, VkDeviceMemory memory, VkImageView view);

	virtual ~VulkanImage() override;
};

using VulkanImageRef = std::shared_ptr<VulkanImage>;