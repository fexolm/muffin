#pragma once

#include <vulkan/vulkan.h>
#include "RHI.h"

class VulkanDescriptorSet : public RHIDescriptorSet {
public:
    VulkanDescriptorSet(VkDevice device, VkDescriptorPool descriptorPool, const VkDescriptorSetLayout *layouts);

    VkDescriptorSet DescriptorSetHandle() const;

    virtual ~VulkanDescriptorSet() override;

    virtual void Update(int binding, const RHIBufferRef &buf, int size) override;

    virtual void Update(int binding, Image &image, Sampler &sampler) override;

private:
    VkDescriptorSet descriptorSetHandle;
    VkDescriptorPool descriptorPoolHandle;
    VkDevice deviceHandle;

    RHIBufferRef bufferRef;
};