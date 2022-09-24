#pragma once

#include "../RHI.h"
#include "VulkanDescriptorPool.h"

#include <vulkan/vulkan.h>

class VulkanDescriptorSet : public RHIResource
{
public:
	VulkanDescriptorSet(VulkanDeviceRef device,
		VulkanDescriptorPoolRef descriptorPool,
		const VkDescriptorSetLayout* layouts);

	VkDescriptorSet DescriptorSetHandle() const;

	virtual ~VulkanDescriptorSet() override;

	void Update(int binding, const RHIBufferRef& buf, int size);

	void Update(int binding, class VulkanImage& image,
		class VulkanSampler& sampler);

private:
	VulkanDeviceRef device;
	VkDescriptorSet descriptorSet;
	VulkanDescriptorPoolRef descriptorPool;

	RHIBufferRef bufferRef;
};

using VulkanDescriptorSetRef = std::shared_ptr<VulkanDescriptorSet>;