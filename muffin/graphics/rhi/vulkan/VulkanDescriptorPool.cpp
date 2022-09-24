#include "VulkanDescriptorPool.h"

VulkanDescriptorPool::VulkanDescriptorPool(VulkanDeviceRef device,
	VkDescriptorPool descriptorPool)
	: device(device), descriptorPool(descriptorPool) {}

VulkanDescriptorPool::~VulkanDescriptorPool()
{
	vkDestroyDescriptorPool(device->Device(), descriptorPool, nullptr);
}

VkDescriptorPool VulkanDescriptorPool::Handle()
{
	return descriptorPool;
}
