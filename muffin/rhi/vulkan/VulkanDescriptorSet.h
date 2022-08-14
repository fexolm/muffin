#pragma once

#include <vulkan/vulkan.h>
#include "../RHI.h"

class VulkanDescriptorSet : public RHIResource {
public:
    VulkanDescriptorSet(VkDevice device, VkDescriptorPool descriptorPool, const VkDescriptorSetLayout *layouts);

    VkDescriptorSet DescriptorSetHandle() const;

    virtual ~VulkanDescriptorSet() override;

    void Update(int binding, const RHIBufferRef &buf, int size);

    void Update(int binding, class VulkanImage &image, class VulkanSampler &sampler);

private:
    VkDescriptorSet descriptorSetHandle;
    VkDescriptorPool descriptorPoolHandle;
    VkDevice deviceHandle;

    RHIBufferRef bufferRef;
};

using VulkanDescriptorSetRef = std::shared_ptr<VulkanDescriptorSet>;