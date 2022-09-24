#include "VulkanRenderPass.h"

VulkanRenderPass::VulkanRenderPass(VulkanDeviceRef device, VkRenderPass renderPass)
	: device(device), renderPass(renderPass) {}

const VkRenderPass& VulkanRenderPass::RenderPass()
{
	return renderPass;
}

VulkanRenderPass::~VulkanRenderPass()
{
	vkDestroyRenderPass(device->Device(), renderPass, nullptr);
}