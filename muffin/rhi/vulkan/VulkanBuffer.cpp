#include <stdexcept>
#include "VulkanBuffer.h"
#include "VulkanRHI.h"

uint32_t
FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        if (typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

VulkanBuffer::VulkanBuffer(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t size, const BufferInfo &info) {
    deviceHandle = device;
    VkBufferCreateInfo bufferInfo;
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.queueFamilyIndexCount = 0;
    bufferInfo.pQueueFamilyIndices = nullptr;
    bufferInfo.flags = 0;
    bufferInfo.pNext = nullptr;

    switch (info.usage) {
        case BufferUsage::Index:
            bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            break;
        case BufferUsage::Vertex:
            bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            break;
        case BufferUsage::Uniform:
            bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            break;
        case BufferUsage::Staging:
            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            break;
    }

    VULKAN_RHI_SAFE_CALL(vkCreateBuffer(deviceHandle, &bufferInfo, nullptr, &bufferHandle));

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(deviceHandle, bufferHandle, &memoryRequirements);

    VkMemoryAllocateInfo allocInfo;
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memoryRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memoryRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    allocInfo.pNext = nullptr;

    VULKAN_RHI_SAFE_CALL(vkAllocateMemory(deviceHandle, &allocInfo, nullptr, &allocHandle));
    vkBindBufferMemory(deviceHandle, bufferHandle, allocHandle, 0);
}

VulkanBuffer::~VulkanBuffer() {
    vkFreeMemory(deviceHandle, allocHandle, nullptr);
    vkDestroyBuffer(deviceHandle, bufferHandle, nullptr);
}

VkBuffer VulkanBuffer::BufferHandle() const {
    return bufferHandle;
}

VkDeviceMemory VulkanBuffer::AllocHandle() const {
    return allocHandle;
}

void VulkanBuffer::Write(void *data, uint32_t size) {
    void *devicePtr;
    vkMapMemory(deviceHandle, allocHandle, 0, size, 0, &devicePtr);
    memcpy(devicePtr, data, size);
    vkUnmapMemory(deviceHandle, allocHandle);
}
