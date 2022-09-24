#pragma once

#include "VulkanDevice.h"

#include <vulkan/vulkan.h>

class VulkanRenderPass
{
public:
	VulkanRenderPass(VulkanDeviceRef device, VkRenderPass renderPass);

	const VkRenderPass& RenderPass();

	~VulkanRenderPass();

private:
	VulkanDeviceRef device;

	VkRenderPass renderPass;
};

using VulkanRenderPassRef = std::shared_ptr<VulkanRenderPass>;