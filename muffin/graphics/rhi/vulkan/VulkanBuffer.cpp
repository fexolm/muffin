#include "VulkanBuffer.h"
#include "VulkanRHI.h"

#include <stdexcept>

uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
	VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
		if (typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

VulkanBuffer::VulkanBuffer(VulkanDeviceRef device,
	VkPhysicalDevice physicalDevice, uint32_t size,
	const BufferInfo& info)
	: device(device)
{
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

	VULKAN_RHI_SAFE_CALL(
		vkCreateBuffer(device->Device(), &bufferInfo, nullptr, &buffer));

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(device->Device(), buffer, &memoryRequirements);

	VkMemoryAllocateInfo allocInfo;
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memoryRequirements.size;
	allocInfo.memoryTypeIndex =
		FindMemoryType(physicalDevice, memoryRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	allocInfo.pNext = nullptr;

	VULKAN_RHI_SAFE_CALL(
		vkAllocateMemory(device->Device(), &allocInfo, nullptr, &alloc));
	vkBindBufferMemory(device->Device(), buffer, alloc, 0);
}

VulkanBuffer::~VulkanBuffer()
{
	vkFreeMemory(device->Device(), alloc, nullptr);
	vkDestroyBuffer(device->Device(), buffer, nullptr);
}

VkBuffer VulkanBuffer::Buffer() const
{
	return buffer;
}

VkDeviceMemory VulkanBuffer::Memory() const
{
	return alloc;
}

void VulkanBuffer::Write(void* data, uint32_t size)
{
	void* devicePtr;
	vkMapMemory(device->Device(), alloc, 0, size, 0, &devicePtr);
	memcpy(devicePtr, data, size);
	vkUnmapMemory(device->Device(), alloc);
}
