#include "VulkanRHI.h"
#include "VulkanBuffer.h"
#include "VulkanDescriptorSet.h"
#include "VulkanGraphicsPipeline.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <limits>
#include <spirv_cross/spirv_cross.hpp>

uint32_t
findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
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

VkSurfaceKHR createSurface(VkInstance instance, SDL_Window* window)
{
	VkSurfaceKHR surface;
	if (!SDL_Vulkan_CreateSurface(window, instance, (SDL_vulkanSurface*)&surface)) {
		throw;
	}
	return surface;
}

VulkanCommandPoolRef createCommandPool(VulkanDeviceRef device, uint32_t graphicsFamilyIdx)
{
	VkCommandPoolCreateInfo commandPoolCreateInfo{};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = graphicsFamilyIdx;
	commandPoolCreateInfo.pNext = nullptr;

	VkCommandPool commandPool;
	vkCreateCommandPool(device->Device(), &commandPoolCreateInfo, nullptr, &commandPool);

	return VulkanCommandPoolRef(new VulkanCommandPool(device, commandPool));
}

VkCommandBuffer createCommandBuffer(VkDevice device, VkCommandPool commandPool)
{
	VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = 1;
	commandBufferAllocateInfo.pNext = nullptr;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer);
	return commandBuffer;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D
chooseSwapExtent(VkSurfaceCapabilitiesKHR capabilities, uint32_t width, uint32_t height)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	} else {

		VkExtent2D actualExtent = { width, height };

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
			capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
			capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

VkSwapchainKHR
createSwapchain(VkSurfaceKHR surface, VkDevice device,
	VkSurfaceFormatKHR surfaceFormat, VkPresentModeKHR presentMode,
	VkSurfaceCapabilitiesKHR caps,
	VkExtent2D extent, uint32_t graphicsFamilyIdx, uint32_t presentFamilyIdx)
{
	uint32_t imageCount = caps.minImageCount + 1;

	VkSwapchainCreateInfoKHR swapchainCreateInfo{};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageFormat = surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent = extent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	uint32_t queueFamilyIndices[] = { graphicsFamilyIdx, presentFamilyIdx };
	if (graphicsFamilyIdx == presentFamilyIdx) {
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0;
		swapchainCreateInfo.pQueueFamilyIndices = nullptr;
	} else {
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = 2;
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}

	swapchainCreateInfo.preTransform = caps.currentTransform;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.clipped = true;
	swapchainCreateInfo.oldSwapchain = nullptr;
	swapchainCreateInfo.flags = 0;
	swapchainCreateInfo.pNext = nullptr;

	VkSwapchainKHR swapchain;

	vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain);
	return swapchain;
}

std::vector<VkImageView>
createSwapchainImageViews(VkSwapchainKHR swapchain, VkDevice device,
	VkSurfaceFormatKHR surfaceFormat, VkExtent2D extent)
{

	uint32_t imagesCount = 0;
	vkGetSwapchainImagesKHR(device, swapchain, &imagesCount, nullptr);

	std::vector<VkImage> images(imagesCount);

	vkGetSwapchainImagesKHR(device, swapchain, &imagesCount, images.data());

	std::vector<VkImageView> imageViews;

	for (auto image : images) {
		VkImageViewCreateInfo imageViewCreateInfo{};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = image;
		imageViewCreateInfo.format = surfaceFormat.format;
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		imageViewCreateInfo.flags = 0;
		imageViewCreateInfo.pNext = nullptr;

		VkImageView imageView;

		vkCreateImageView(device, &imageViewCreateInfo, nullptr, &imageView);

		imageViews.emplace_back(imageView);
	}
	return imageViews;
}

VulkanDescriptorPoolRef createDescriptorPool(VulkanDeviceRef device)
{
	VkDescriptorPoolSize poolSizes[2];
	poolSizes[0].descriptorCount = 4096;
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

	poolSizes[1].descriptorCount = 4096;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	VkDescriptorPoolCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.poolSizeCount = 2;
	createInfo.pPoolSizes = poolSizes;
	createInfo.maxSets = 1000;
	createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	createInfo.pNext = nullptr;

	VkDescriptorPool descriptorPool;

	vkCreateDescriptorPool(device->Device(), &createInfo, nullptr, &descriptorPool);

	return VulkanDescriptorPoolRef(new VulkanDescriptorPool(device, descriptorPool));
}

VulkanImageRef
createImageImpl(VulkanDeviceRef device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height,
	VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
	VkMemoryPropertyFlags memoryPorperties, VkImageAspectFlagBits aspectMask)
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0;
	imageInfo.pNext = nullptr;

	VkImage image;

	vkCreateImage(device->Device(), &imageInfo, nullptr, &image);

	VkMemoryRequirements memReq;
	vkGetImageMemoryRequirements(device->Device(), image, &memReq);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memReq.size;
	allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memReq.memoryTypeBits, memoryPorperties);
	allocInfo.pNext = nullptr;

	VkDeviceMemory memory;
	vkAllocateMemory(device->Device(), &allocInfo, nullptr, &memory);

	vkBindImageMemory(device->Device(), image, memory, 0);

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectMask;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;
	viewInfo.flags = 0;
	viewInfo.pNext = nullptr;

	VkImageView imageView;
	vkCreateImageView(device->Device(), &viewInfo, nullptr, &imageView);

	return std::make_shared<VulkanImage>(device, image, memory, imageView);
}

VkDescriptorSetLayout
createDescriptorSetLayout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
	VkDescriptorSetLayoutCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.bindingCount = bindings.size();
	createInfo.pBindings = bindings.data();
	createInfo.flags = 0;
	createInfo.pNext = nullptr;

	VkDescriptorSetLayout descriptorSetLayout;
	vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &descriptorSetLayout);

	return descriptorSetLayout;
}

VkPipelineLayout
createPipelineLayout(VkDevice& device, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts)
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;
	pipelineLayoutInfo.flags = 0;
	pipelineLayoutInfo.pNext = nullptr;

	VkPipelineLayout pipelineLayout;

	vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);

	return pipelineLayout;
}

bool hasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void transitionImageLayout(VulkanRHI& rhi, VkImage img, VkFormat format, VkImageLayout oldLayout,
	VkImageLayout newLayout)
{
	auto cmdList = rhi.CreateCommandList();
	VulkanCommandList& vulkanCommandList = static_cast<VulkanCommandList&>(*cmdList);
	vulkanCommandList.Begin();

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destStage;

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = img;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (hasStencilComponent(format)) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	} else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_NONE;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_NONE;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	} else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	barrier.pNext = nullptr;

	vkCmdPipelineBarrier(vulkanCommandList.commandBuffer, sourceStage, destStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	vulkanCommandList.End();
	rhi.SubmitAndWaitIdle(cmdList);
}

VkFormat findSupportedFormat(VkPhysicalDevice physicalDevice, const std::vector<VkFormat>& candidates,
	VkImageTiling tiling, VkFormatFeatureFlagBits features)
{
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		} else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}
	throw std::runtime_error("failed to find supported format!");
}

VkFormat findDepthFormat(VkPhysicalDevice physicalDevice)
{
	return findSupportedFormat(physicalDevice,
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VulkanImageRef
createDepthImage(VulkanRHI& rhi, VulkanDeviceRef device, VkPhysicalDevice physicalDevice, uint32_t width,
	uint32_t height)
{
	VkFormat depthFormat = findDepthFormat(physicalDevice);
	VulkanImageRef image = createImageImpl(device, physicalDevice, width, height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

	transitionImageLayout(rhi, image->image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	return image;
}

VulkanRenderPassRef VulkanRHI::createRenderPass(int imgIdx)
{
	if (renderPassCache.count(imgIdx)) {
		return renderPassCache.at(imgIdx);
	}

	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = surfaceFormat.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	colorAttachment.flags = 0;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = findDepthFormat(device->PhysicalDevice());
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	depthAttachment.flags = 0;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.inputAttachmentCount = 0;
	subpass.preserveAttachmentCount = 0;
	subpass.flags = 0;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = VK_ACCESS_NONE;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = attachments.size();
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;
	renderPassInfo.flags = 0;
	renderPassInfo.pNext = nullptr;

	VkRenderPass renderPass{};

	vkCreateRenderPass(device->Device(), &renderPassInfo, nullptr, &renderPass);

	renderPassCache.emplace(imgIdx, VulkanRenderPassRef(new VulkanRenderPass(device, renderPass)));

	return renderPassCache.at(imgIdx);
}

RHIGraphicsPipelineRef VulkanRHI::CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& info)
{
	return RHIGraphicsPipelineRef(
		new VulkanGraphicsPipeline(device, extent, surfaceFormat.format, findDepthFormat(device->PhysicalDevice()), info));
}

VkFramebuffer
VulkanRHI::createFramebuffer(VulkanRenderPassRef renderPass, const VulkanRenderTarget& renderTarget)
{
	if (frameBuffersCache.count(renderTarget.imageIdx)) {
		return frameBuffersCache.at(renderTarget.imageIdx);
	}

	VkImageView attachments[] = {
		renderTarget.swapchainImg, depthImage->view
	};
	VkFramebufferCreateInfo framebufferInfo{};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = renderPass->RenderPass();
	framebufferInfo.attachmentCount = 2;
	framebufferInfo.pAttachments = attachments;
	framebufferInfo.width = extent.width;
	framebufferInfo.height = extent.height;
	framebufferInfo.layers = 1;
	framebufferInfo.flags = 0;
	framebufferInfo.pNext = nullptr;

	VkFramebuffer framebuffer;

	vkCreateFramebuffer(device->Device(), &framebufferInfo, nullptr, &framebuffer);

	frameBuffersCache.emplace(renderTarget.imageIdx, framebuffer);
	return frameBuffersCache.at(renderTarget.imageIdx);
}

std::vector<VkSurfaceFormatKHR> getSurfaceFormats(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	uint32_t formatsCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, nullptr);

	std::vector<VkSurfaceFormatKHR> surfaceFormats(formatsCount);

	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatsCount, surfaceFormats.data());
	return surfaceFormats;
}

std::vector<VkPresentModeKHR> getSurfacePresentModes(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	uint32_t modesCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &modesCount, nullptr);

	std::vector<VkPresentModeKHR> presentModes(modesCount);

	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &modesCount, presentModes.data());

	return presentModes;
}

VulkanRHI::VulkanRHI()
{
	uint32_t extensionsCount;
	SDL_Vulkan_GetInstanceExtensions(window.window, &extensionsCount, nullptr);
	SDL_Vulkan_GetInstanceExtensions(window.window, &extensionsCount, nullptr);
	std::vector<const char*> enabledExtensions(extensionsCount);
	SDL_Vulkan_GetInstanceExtensions(window.window, &extensionsCount, enabledExtensions.data());

	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	instance = VulkanInstanceRef(new VulkanInstance(enabledExtensions));

	surface = createSurface(instance->Instance(), window.window);

	device = VulkanDeviceRef(new VulkanDevice(instance, deviceExtensions, surface));

	surfaceFormat = chooseSwapSurfaceFormat(getSurfaceFormats(device->PhysicalDevice(), surface));
	presentMode = chooseSwapPresentMode(getSurfacePresentModes(device->PhysicalDevice(), surface));

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->PhysicalDevice(), surface, &caps);
	extent = chooseSwapExtent(caps, window.width, window.height);

	swapchain = createSwapchain(surface, device->Device(), surfaceFormat, presentMode, caps, extent,
		device->GraphicsFamily(), device->PresentFamily());

	swapchainImageViews = createSwapchainImageViews(swapchain, device->Device(), surfaceFormat, extent);

	commandPool = createCommandPool(device, device->GraphicsFamily());

	descriptorPool = createDescriptorPool(device);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.flags = 0;
	semaphoreInfo.pNext = nullptr;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	fenceInfo.pNext = nullptr;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkCreateSemaphore(device->Device(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]);
		vkCreateSemaphore(device->Device(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]);
		vkCreateFence(device->Device(), &fenceInfo, nullptr, &inFlightFences[i]);
	}
	currentFrame = 0;

	depthImage = createDepthImage(*this, device, device->PhysicalDevice(), extent.width, extent.height);
}

#include <iostream>

static inline VkFormat toVkBufferFormat(VertexElementType Type)
{
	switch (Type) {
		case Float1:
			return VK_FORMAT_R32_SFLOAT;
		case Float2:
			return VK_FORMAT_R32G32_SFLOAT;
		case Float3:
			return VK_FORMAT_R32G32B32_SFLOAT;
		case PackedNormal:
			return VK_FORMAT_R8G8B8A8_SNORM;
		case UByte4:
			return VK_FORMAT_R8G8B8A8_UINT;
		case UByte4N:
			return VK_FORMAT_R8G8B8A8_UNORM;
		case Color:
			return VK_FORMAT_B8G8R8A8_UNORM;
		case Short2:
			return VK_FORMAT_R16G16_SINT;
		case Short4:
			return VK_FORMAT_R16G16B16A16_SINT;
		case Short2N:
			return VK_FORMAT_R16G16_SNORM;
		case Half2:
			return VK_FORMAT_R16G16_SFLOAT;
		case Half4:
			return VK_FORMAT_R16G16B16A16_SFLOAT;
		case Short4N: // 4 X 16 bit word: normalized
			return VK_FORMAT_R16G16B16A16_SNORM;
		case UShort2:
			return VK_FORMAT_R16G16_UINT;
		case UShort4:
			return VK_FORMAT_R16G16B16A16_UINT;
		case UShort2N: // 16 bit word normalized to (value/65535.0:value/65535.0:0:0:1)
			return VK_FORMAT_R16G16_UNORM;
		case UShort4N: // 4 X 16 bit word unsigned: normalized
			return VK_FORMAT_R16G16B16A16_UNORM;
		case Float4:
			return VK_FORMAT_R32G32B32A32_SFLOAT;
		case URGB10A2N:
			return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
		case UInt:
			return VK_FORMAT_R32_UINT;
		default:
			break;
	}

	throw std::runtime_error("Undefined vertex-element format conversion");
}

uint32_t getTypeSize(VertexElementType type)
{
	switch (type) {
		case VertexElementType::Float1:
			return 4;
		case VertexElementType::Float2:
			return 8;
		case VertexElementType::Float3:
			return 12;
		case VertexElementType::Float4:
			return 16;
		default:
			throw std::runtime_error("Unsuported type");
	}
}

VertexElementType spirvToVertexElementType(spirv_cross::SPIRType type)
{
	if (type.basetype == spirv_cross::SPIRType::Float) {
		if (type.vecsize == 1) {
			return VertexElementType::Float1;
		}
		if (type.vecsize == 2) {
			return VertexElementType::Float2;
		}
		if (type.vecsize == 3) {
			return VertexElementType::Float3;
		}
		if (type.vecsize == 4) {
			return VertexElementType::Float4;
		}
	}
	return VertexElementType::None;
}

RHIShaderRef VulkanRHI::CreateShader(const std::vector<uint32_t>& code, ShaderType type)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size() * sizeof(uint32_t);
	createInfo.pCode = code.data();
	createInfo.flags = 0;
	createInfo.pNext = nullptr;

	VkShaderModule shaderModule;

	vkCreateShaderModule(device->Device(), &createInfo, nullptr, &shaderModule);

	spirv_cross::Compiler comp(code);

	auto resources = comp.get_shader_resources();

	auto res = std::make_shared<VulkanShader>(device, shaderModule);

	if (type == ShaderType::Vertex) {
		int i = 0;
		for (auto& b : resources.stage_inputs) {
			VkVertexInputAttributeDescription attributeDescription{};
			attributeDescription.binding = i;
			attributeDescription.location = comp.get_decoration(b.id, spv::DecorationLocation);
			attributeDescription.offset = 0;
			VertexElementType elementType = spirvToVertexElementType(comp.get_type(b.type_id));
			attributeDescription.format = toVkBufferFormat(elementType);

			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = i;
			bindingDescription.stride = getTypeSize(elementType);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			res->vertexAttributes.push_back(attributeDescription);
			res->vertexBindings.push_back(bindingDescription);

			i++;
		}
	}

	for (auto& ub : resources.uniform_buffers) {
		int binding = comp.get_decoration(ub.id, spv::DecorationBinding);
		int set = comp.get_decoration(ub.id, spv::DecorationDescriptorSet);

		res->params[ub.name] = { set, binding };

		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.binding = binding;
		layoutBinding.descriptorCount = 1;
		layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		switch (type) {
			case ShaderType::Vertex:
				layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
				break;
			case ShaderType::Fragment:
				layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
				break;
		}
		layoutBinding.pImmutableSamplers = nullptr;

		res->bindings[set].push_back(layoutBinding);
	}

	for (auto& ub : resources.sampled_images) {
		int binding = comp.get_decoration(ub.id, spv::DecorationBinding);
		int set = comp.get_decoration(ub.id, spv::DecorationDescriptorSet);

		res->params[ub.name] = { set, binding };

		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.binding = binding;
		layoutBinding.descriptorCount = 1;
		layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

		switch (type) {
			case ShaderType::Vertex:
				layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
				break;
			case ShaderType::Fragment:
				layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
				break;
		}
		layoutBinding.pImmutableSamplers = nullptr;
		res->bindings[set].push_back(layoutBinding);
	}

	return res;
}

void VulkanRHI::Submit(RHICommandListRef& commandList)
{
	VulkanCommandList& vulkanCommandList = static_cast<VulkanCommandList&>(*commandList);
	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &imageAvailableSemaphores[currentFrame];

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &vulkanCommandList.commandBuffer;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &renderFinishedSemaphores[currentFrame];

	submitInfo.pNext = nullptr;

	vkQueueSubmit(device->GraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]);

	inFlightResources.emplace(currentFrame, commandList);
}

RHICommandListRef VulkanRHI::CreateCommandList()
{
	VkCommandBuffer commandBuffer = createCommandBuffer(device->Device(), commandPool->CommandPool());
	vkResetCommandBuffer(commandBuffer, 0);

	return RHICommandListRef(new VulkanCommandList(device, commandPool, commandBuffer, this));
}

VkExtent2D VulkanRHI::getExtent()
{
	return extent;
}

RHIRenderTargetRef VulkanRHI::BeginFrame()
{
	vkWaitForFences(device->Device(), 1, &inFlightFences[currentFrame], true, UINT64_MAX);
	inFlightResources.erase(currentFrame);
	vkResetFences(device->Device(), 1, &inFlightFences[currentFrame]);

	vkAcquireNextImageKHR(device->Device(), swapchain, UINT64_MAX, imageAvailableSemaphores[currentFrame], nullptr, &currentSwapchainImgIdx);

	return RHIRenderTargetRef(
		new VulkanRenderTarget{ .swapchainImg = swapchainImageViews[currentSwapchainImgIdx], .imageIdx = currentSwapchainImgIdx });
}

void VulkanRHI::EndFrame()
{
	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapchain;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &renderFinishedSemaphores[currentFrame];
	presentInfo.pResults = nullptr;
	presentInfo.pImageIndices = &currentSwapchainImgIdx;
	presentInfo.pNext = nullptr;

	vkQueuePresentKHR(device->GraphicsQueue(), &presentInfo);

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

RHIBufferRef VulkanRHI::CreateBuffer(size_t size, const BufferInfo& info)
{
	return RHIBufferRef(new VulkanBuffer(device, device->PhysicalDevice(), size, info));
}

VulkanDescriptorSetRef VulkanRHI::CreateDescriptorSet(const RHIGraphicsPipelineRef& pipeline, int num)
{
	VulkanGraphicsPipeline* vulkanPipeline = static_cast<VulkanGraphicsPipeline*>(pipeline.get());
	VkDescriptorSetLayout layout = vulkanPipeline->DescriptorLayouts()[num];
	return VulkanDescriptorSetRef(new VulkanDescriptorSet(device, descriptorPool, &layout));
}

RHITextureRef VulkanRHI::CreateTexture(uint32_t width, uint32_t height)
{
	return createImageImpl(device, device->PhysicalDevice(), width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
}

void VulkanRHI::CopyBufferToTexture(const RHIBufferRef& buf, RHITextureRef& texture, uint32_t width, uint32_t height)
{
	VulkanBuffer* buffer = static_cast<VulkanBuffer*>(buf.get());
	VulkanImage* image = static_cast<VulkanImage*>(texture.get());
	transitionImageLayout(*this, image->image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	auto cmdList = CreateCommandList();

	VulkanCommandList& vulkanCommmandList = static_cast<VulkanCommandList&>(*cmdList);
	vulkanCommmandList.Begin();

	VkBufferImageCopy region;
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;
	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(vulkanCommmandList.commandBuffer, buffer->Buffer(), image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	vulkanCommmandList.End();
	SubmitAndWaitIdle(cmdList);

	transitionImageLayout(*this, image->image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void VulkanRHI::SubmitAndWaitIdle(RHICommandListRef& commandList)
{
	VulkanCommandList& vulkanCommmandList = static_cast<VulkanCommandList&>(*commandList);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &vulkanCommmandList.commandBuffer;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pNext = nullptr;

	vkQueueSubmit(device->GraphicsQueue(), 1, &submitInfo, nullptr);
	vkDeviceWaitIdle(device->Device());
}

RHISamplerRef VulkanRHI::CreateSampler()
{
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(device->PhysicalDevice(), &properties);

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.anisotropyEnable = true;
	samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = false;
	samplerInfo.compareEnable = false;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.f;
	samplerInfo.minLod = 0.f;
	samplerInfo.maxLod = 0.f;
	samplerInfo.flags = 0;
	samplerInfo.pNext = nullptr;

	VkSampler sampler;

	vkCreateSampler(device->Device(), &samplerInfo, nullptr, &sampler);
	return RHISamplerRef(new VulkanSampler(device, sampler));
}

void VulkanRHI::WaitIdle()
{
	vkDeviceWaitIdle(device->Device());
}

VulkanRHI::~VulkanRHI()
{
	for (auto& p : frameBuffersCache) {
		vkDestroyFramebuffer(device->Device(), p.second, nullptr);
	}

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device->Device(), imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(device->Device(), renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(device->Device(), inFlightFences[i], nullptr);
	}

	for (VkImageView view : swapchainImageViews) {
		vkDestroyImageView(device->Device(), view, nullptr);
	}

	vkDestroySwapchainKHR(device->Device(), swapchain, nullptr);

	vkDestroySurfaceKHR(instance->Instance(), surface, nullptr);
}