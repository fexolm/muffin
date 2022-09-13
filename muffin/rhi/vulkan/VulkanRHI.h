#pragma once

#include "../RHI.h"
#include "VulkanDescriptorPool.h"
#include "VulkanDescriptorSet.h"
#include "VulkanDevice.h"
#include "VulkanRenderPass.h"

#include <SDL2/SDL.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>

struct VulkanWindow
{
	SDL_Window* window;
	int width;
	int height;

	VulkanWindow();

	~VulkanWindow();
};

struct VulkanImage : RHITexture
{
	VulkanDeviceRef device;
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;

	VulkanImage(VulkanDeviceRef device, VkImage img, VkDeviceMemory memory, VkImageView view);

	virtual ~VulkanImage() override;
};

using VulkanImageRef = std::shared_ptr<VulkanImage>;

struct VulkanSampler : RHISampler
{
	VulkanDeviceRef device;
	VkSampler sampler;

	VulkanSampler(VulkanDeviceRef device, VkSampler sampler);

	virtual ~VulkanSampler() override;
};

struct DescriptorSetBindingPoint
{
	int set;
	int binding;
};

struct VulkanShader : public RHIShader
{
	explicit VulkanShader(VulkanDeviceRef device, VkShaderModule module);

	virtual ~VulkanShader() override;

	VulkanDeviceRef device;
	VkShaderModule module;

	std::map<int, std::vector<VkDescriptorSetLayoutBinding>> bindings;
	std::vector<VkVertexInputAttributeDescription> vertexAttributes;
	std::vector<VkVertexInputBindingDescription> vertexBindings;

	std::unordered_map<std::string, DescriptorSetBindingPoint> params;
};

struct VulkanRenderTarget : public RHIRenderTarget
{
	VkImageView swapchainImg;
	uint32_t imageIdx;
};

class VulkanCommandPool
{
public:
	VulkanCommandPool(VulkanDeviceRef device, VkCommandPool commandPool);

	const VkCommandPool& CommandPool();

	~VulkanCommandPool();

private:
	VulkanDeviceRef device;
	VkCommandPool commandPool;
};

using VulkanCommandPoolRef = std::shared_ptr<VulkanCommandPool>;

struct VulkanCommandList : public RHICommandList
{
	explicit VulkanCommandList(VulkanDeviceRef device, VulkanCommandPoolRef commandPool, VkCommandBuffer commandBuffer, class VulkanRHI* rhi);

	virtual ~VulkanCommandList() override;

	virtual void Begin() override;

	virtual void End() override;

	virtual void BindPipeline(const RHIGraphicsPipelineRef& pipeline) override;

	virtual void BeginRenderPass(const RHIRenderTargetRef& renderTarget) override;

	virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
		uint32_t firstInstance) override;

	virtual void SetViewport() override;

	virtual void SetScissors() override;

	virtual void EndRenderPass() override;

	virtual void BindVertexBuffer(const RHIBufferRef& buf, int binding) override;

	virtual void BindIndexBuffer(const RHIBufferRef& buf) override;

	virtual void BindUniformBuffer(const std::string& name, const RHIBufferRef& buffer, int size) override;

	virtual void BindTexture(const std::string& name, const RHITextureRef& texture, const RHISamplerRef& sampler);

	void BindDescriptorSet(const RHIGraphicsPipelineRef& pipeline,
		const VulkanDescriptorSetRef& descriptorSet, int binding);

	class VulkanRHI* rhi;

	VulkanDeviceRef device;
	VulkanCommandPoolRef commandPool;
	VkCommandBuffer commandBuffer;

	std::vector<RHIResourceRef> ownedResources;

	std::vector<VulkanDescriptorSetRef> currentDescriptorSets;

	RHIGraphicsPipelineRef currentPipeline;
};

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

#define VULKAN_RHI_SAFE_CALL(Result)   \
	do {                               \
		if ((Result) != VK_SUCCESS) {} \
	} while (0)
