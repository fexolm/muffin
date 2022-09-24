#pragma once

#include "VulkanDevice.h"
#include "muffin/graphics/rhi/RHI.h"

#include <vulkan/vulkan.h>

struct VulkanSampler : RHISampler
{
	VulkanDeviceRef device;
	VkSampler sampler;

	VulkanSampler(VulkanDeviceRef device, VkSampler sampler);

	virtual ~VulkanSampler() override;
};