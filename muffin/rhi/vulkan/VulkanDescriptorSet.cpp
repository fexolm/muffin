#include "VulkanDescriptorSet.h"
#include "VulkanBuffer.h"
#include "VulkanRHI.h"

VulkanDescriptorSet::VulkanDescriptorSet(VkDevice device, VkDescriptorPool descriptorPool, const VkDescriptorSetLayout *layout) {
    deviceHandle = device;
    descriptorPoolHandle = descriptorPool;
    VkDescriptorSetAllocateInfo allocInfo;
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = layout;
    allocInfo.pNext = nullptr;

    VULKAN_RHI_SAFE_CALL(vkAllocateDescriptorSets(deviceHandle, &allocInfo, &descriptorSetHandle));
}

VulkanDescriptorSet::~VulkanDescriptorSet() {
    vkFreeDescriptorSets(deviceHandle, descriptorPoolHandle, 1, &descriptorSetHandle);
}

VkDescriptorSet VulkanDescriptorSet::DescriptorSetHandle() const {
    return descriptorSetHandle;
}

void VulkanDescriptorSet::Update(int binding, const RHIBufferRef &buf, int size) {
    VulkanBuffer *buffer = static_cast<VulkanBuffer *>(buf.get());
    bufferRef = buf;

    VkDescriptorBufferInfo bufferInfo;
    bufferInfo.buffer = buffer->BufferHandle();
    bufferInfo.offset = 0;
    bufferInfo.range = size;

    VkWriteDescriptorSet write;
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptorSetHandle;
    write.dstBinding = binding;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.descriptorCount = 1;
    write.pBufferInfo = &bufferInfo;
    write.pImageInfo = nullptr;
    write.pTexelBufferView = nullptr;
    write.pNext = nullptr;

    vkUpdateDescriptorSets(deviceHandle, 1, &write, 0, nullptr);
}

void VulkanDescriptorSet::Update(int binding, VulkanImage &image, VulkanSampler &sampler) {
    VkDescriptorImageInfo imageInfo;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = *image.view;
    imageInfo.sampler = *sampler.sampler;

    VkWriteDescriptorSet write;
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptorSetHandle;
    write.dstBinding = binding;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;
    write.pNext = nullptr;

    vkUpdateDescriptorSets(deviceHandle, 1, &write, 0, nullptr);
}
