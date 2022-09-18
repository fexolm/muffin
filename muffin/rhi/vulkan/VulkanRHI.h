#pragma once

#include "muffin/rhi/RHI.h"
#include "Shared.h"
#include "VulkanCommandList.h"
#include "VulkanCommandPool.h"
#include "VulkanDescriptorPool.h"
#include "VulkanDescriptorSet.h"
#include "VulkanDevice.h"
#include "VulkanImage.h"
#include "VulkanRenderPass.h"
#include "VulkanRenderTarget.h"
#include "VulkanSampler.h"
#include "VulkanShader.h"
#include "VulkanWindow.h"

#include <SDL2/SDL.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>

const int MAX_FRAMES_IN_FLIGHT = 2;

class VulkanRHI : public RHIDriver
{

public:
	VulkanRHI();

	virtual ~VulkanRHI() override;

	virtual RHIShaderRef CreateShader(const std::vector<uint32_t>& code, ShaderType type) override;

	virtual RHIBufferRef CreateBuffer(size_t size, const BufferInfo& info) override;

	virtual RHIGraphicsPipelineRef CreateGraphicsPipeline(const GraphicsPipelineCreateInfo& info) override;

	virtual RHICommandListRef CreateCommandList() override;

	virtual void Submit(RHICommandListRef& commandList) override;

	virtual void WaitIdle() override;

	virtual void SubmitAndWaitIdle(RHICommandListRef& commandList) override;

	virtual RHIRenderTargetRef BeginFrame() override;

	virtual RHITextureRef CreateTexture(uint32_t width, uint32_t height) override;

	virtual RHISamplerRef CreateSampler() override;

	virtual void CopyBufferToTexture(const RHIBufferRef& buf, RHITextureRef& texture, uint32_t width, uint32_t height) override;

	virtual void EndFrame() override;

	VulkanRenderPassRef createRenderPass(int imgIdx);

	VkFramebuffer createFramebuffer(VulkanRenderPassRef renderPass, const VulkanRenderTarget& renderTarget);

	VkExtent2D getExtent();

	VulkanDescriptorSetRef CreateDescriptorSet(const RHIGraphicsPipelineRef& pipeline, int num);

	void waitIdle();

private:
	VulkanWindow window;

	VulkanInstanceRef instance;
	VulkanDeviceRef device;

	VkSurfaceKHR surface;

	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR presentMode;
	VkSurfaceCapabilitiesKHR caps;
	VkExtent2D extent;
	VkSwapchainKHR swapchain;
	std::vector<VkImageView> swapchainImageViews;

	VulkanCommandPoolRef commandPool;

	VulkanDescriptorPoolRef descriptorPool;

	VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
	VkFence inFlightFences[MAX_FRAMES_IN_FLIGHT];

	std::unordered_map<int, VkFramebuffer> frameBuffersCache;
	std::unordered_map<int, VulkanRenderPassRef> renderPassCache;

	uint32_t currentSwapchainImgIdx;
	uint32_t currentFrame;

	VulkanImageRef depthImage;

	std::unordered_multimap<int, RHIResourceRef> inFlightResources;
};