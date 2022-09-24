#include "VulkanImage.h"

VulkanImage::VulkanImage(VulkanDeviceRef device, VkImage img, VkDeviceMemory memory, VkImageView view)
	: device(device), image(img), memory(memory), view(view)
{
}

VulkanImage::~VulkanImage()
{
	vkDestroyImageView(device->Device(), view, nullptr);
	vkDestroyImage(device->Device(), image, nullptr);
	vkFreeMemory(device->Device(), memory, nullptr);
}