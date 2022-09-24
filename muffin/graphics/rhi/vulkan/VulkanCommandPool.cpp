#include "VulkanCommandPool.h"

VulkanCommandPool::VulkanCommandPool(VulkanDeviceRef device, VkCommandPool commandPool)
	: device(device), commandPool(commandPool)
{
}

const VkCommandPool& VulkanCommandPool::CommandPool()
{
	return commandPool;
}

VulkanCommandPool::~VulkanCommandPool()
{
	vkDestroyCommandPool(device->Device(), commandPool, nullptr);
}