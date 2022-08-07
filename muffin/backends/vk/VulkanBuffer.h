#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include "RHI.h"

class VulkanBuffer : public RHIBuffer {
public:
    VulkanBuffer(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t size, const BufferInfo &info);

    virtual void Write(void *data, uint32_t size) override;

    VkBuffer BufferHandle() const;

    VkDeviceMemory AllocHandle() const;

    virtual ~VulkanBuffer() override;

private:
    VkDevice deviceHandle;
    VkBuffer bufferHandle;
    VkDeviceMemory allocHandle;
};