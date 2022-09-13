#pragma once

#include "../RHI.h"
#include "VulkanDevice.h"

#include <memory>
#include <vulkan/vulkan.h>

class VulkanBuffer : public RHIBuffer
{
public:
	VulkanBuffer(VulkanDeviceRef device, VkPhysicalDevice physicalDevice,
		uint32_t size, const BufferInfo& info);

	virtual void Write(void* data, uint32_t size) override;

	VkBuffer Buffer() const;

	VkDeviceMemory Memory() const;

	virtual ~VulkanBuffer() override;

private:
	VulkanDeviceRef device;
	VkBuffer buffer;
	VkDeviceMemory alloc;
};