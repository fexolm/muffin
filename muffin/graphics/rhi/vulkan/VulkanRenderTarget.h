#pragma once

#include "muffin/graphics/rhi/RHI.h"

#include <vulkan/vulkan.h>

struct VulkanRenderTarget : public RHIRenderTarget
{
	VkImageView swapchainImg;
	uint32_t imageIdx;
};