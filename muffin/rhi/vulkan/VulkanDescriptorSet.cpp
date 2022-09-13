#include "VulkanDescriptorSet.h"
#include "VulkanBuffer.h"
#include "VulkanRHI.h"

VulkanDescriptorSet::VulkanDescriptorSet(VulkanDeviceRef device,
	VulkanDescriptorPoolRef descriptorPool,
	const VkDescriptorSetLayout* layout)
	: device(device), descriptorPool(descriptorPool)
{
	VkDescriptorSetAllocateInfo allocInfo;
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool->Handle();
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layout;
	allocInfo.pNext = nullptr;

	VULKAN_RHI_SAFE_CALL(
		vkAllocateDescriptorSets(device->Device(), &allocInfo, &descriptorSet));
}

VulkanDescriptorSet::~VulkanDescriptorSet()
{
	vkFreeDescriptorSets(device->Device(), descriptorPool->Handle(), 1,
		&descriptorSet);
}

VkDescriptorSet VulkanDescriptorSet::DescriptorSetHandle() const
{
	return descriptorSet;
}

void VulkanDescriptorSet::Update(int binding, const RHIBufferRef& buf,
	int size)
{
	VulkanBuffer* buffer = static_cast<VulkanBuffer*>(buf.get());
	bufferRef = buf;

	VkDescriptorBufferInfo bufferInfo;
	bufferInfo.buffer = buffer->Buffer();
	bufferInfo.offset = 0;
	bufferInfo.range = size;

	VkWriteDescriptorSet write;
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = descriptorSet;
	write.dstBinding = binding;
	write.dstArrayElement = 0;
	write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write.descriptorCount = 1;
	write.pBufferInfo = &bufferInfo;
	write.pImageInfo = nullptr;
	write.pTexelBufferView = nullptr;
	write.pNext = nullptr;

	vkUpdateDescriptorSets(device->Device(), 1, &write, 0, nullptr);
}

void VulkanDescriptorSet::Update(int binding, VulkanImage& image,
	VulkanSampler& sampler)
{
	VkDescriptorImageInfo imageInfo;
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = image.view;
	imageInfo.sampler = sampler.sampler;

	VkWriteDescriptorSet write;
	write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write.dstSet = descriptorSet;
	write.dstBinding = binding;
	write.dstArrayElement = 0;
	write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write.descriptorCount = 1;
	write.pImageInfo = &imageInfo;
	write.pNext = nullptr;

	vkUpdateDescriptorSets(device->Device(), 1, &write, 0, nullptr);
}
