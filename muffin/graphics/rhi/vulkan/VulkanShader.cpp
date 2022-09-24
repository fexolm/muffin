#include "VulkanShader.h"

VulkanShader::VulkanShader(VulkanDeviceRef device, VkShaderModule module)
	: device(device), module(module)
{
}

VulkanShader::~VulkanShader()
{
	vkDestroyShaderModule(device->Device(), module, nullptr);
}