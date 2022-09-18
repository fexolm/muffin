#pragma once

#include "VulkanCommandPool.h"
#include "VulkanDescriptorSet.h"
#include "VulkanDevice.h"
#include "VulkanGraphicsPipeline.h"

#include <vulkan/vulkan.h>

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

	virtual void SetViewport(float offsetX, float offsetY, float width, float height) override;

	virtual void SetScissors(int32_t offsetX, int32_t offsetY, uint32_t width, uint32_t height) override;

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

using VulkanCommandListRef = std::shared_ptr<VulkanCommandList>;