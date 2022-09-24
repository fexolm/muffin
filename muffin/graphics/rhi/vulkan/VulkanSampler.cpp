#include "VulkanSampler.h"

VulkanSampler::VulkanSampler(VulkanDeviceRef device, VkSampler sampler)
	: device(device), sampler(sampler)
{
}

VulkanSampler::~VulkanSampler()
{
	vkDestroySampler(device->Device(), sampler, nullptr);
}
