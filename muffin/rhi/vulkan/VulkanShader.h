#pragma once

#include "VulkanDevice.h"
#include "muffin/rhi/RHI.h"

struct DescriptorSetBindingPoint
{
	int set;
	int binding;
};

struct VulkanShader : public RHIShader
{
	explicit VulkanShader(VulkanDeviceRef device, VkShaderModule module);

	virtual ~VulkanShader() override;

	VulkanDeviceRef device;
	VkShaderModule module;

	std::map<int, std::vector<VkDescriptorSetLayoutBinding>> bindings;
	std::vector<VkVertexInputAttributeDescription> vertexAttributes;
	std::vector<VkVertexInputBindingDescription> vertexBindings;

	std::unordered_map<std::string, DescriptorSetBindingPoint> params;
};