#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <SDL2/SDL.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <memory>
#include <map>
#include <unordered_map>
#include "../RHI.h"
#include "VulkanDescriptorSet.h"

namespace vkr = vk::raii;

struct VulkanWindow {
    SDL_Window *window;
    int width;
    int height;

    VulkanWindow();

    ~VulkanWindow();
};

struct VulkanImage : RHITexture {
    vk::raii::Image image;
    vk::raii::DeviceMemory memory;
    vk::raii::ImageView view;

    VulkanImage(vk::raii::Image &&img, vk::raii::DeviceMemory &&memory, vk::raii::ImageView &&view);
};

using VulkanImageRef = std::shared_ptr<VulkanImage>;

struct VulkanSampler : RHISampler {
    vk::raii::Sampler sampler;

    VulkanSampler(vk::raii::Sampler &&sampler);
};

struct DescriptorSetBindingPoint {
    int set;
    int binding;
};

struct VulkanShader : public RHIShader {
    explicit VulkanShader(vk::raii::ShaderModule &&module);

    vk::raii::ShaderModule module;

    std::map<int, std::vector<VkDescriptorSetLayoutBinding>> bindings;
    std::vector<VkVertexInputAttributeDescription> vertexAttributes;
    std::vector<VkVertexInputBindingDescription> vertexBindings;

    std::unordered_map<std::string, DescriptorSetBindingPoint> params;
};


struct VulkanRenderTarget : public RHIRenderTarget {
    vk::ImageView swapchainImg;
    uint32_t imageIdx;
};

struct VulkanCommandList : public RHICommandList {
    explicit VulkanCommandList(vkr::CommandBuffer &&commandBuffer, class VulkanRHI *rhi);

    virtual void Begin() override;

    virtual void End() override;

    virtual void BindPipeline(const RHIGraphicsPipelineRef &pipeline) override;

    virtual void BeginRenderPass(const RHIRenderTargetRef &renderTarget) override;

    virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                     uint32_t firstInstance) override;

    virtual void SetViewport() override;

    virtual void SetScissors() override;

    virtual void EndRenderPass() override;

    virtual void BindVertexBuffer(const RHIBufferRef &buf, int binding) override;

    virtual void BindIndexBuffer(const RHIBufferRef &buf) override;

    virtual void BindUniformBuffer(const std::string &name, const RHIBufferRef &buffer, int size) override;

    virtual void BindTexture(const std::string &name, const RHITextureRef &texture, const RHISamplerRef &sampler);

    void BindDescriptorSet(const RHIGraphicsPipelineRef &pipeline,
                           const VulkanDescriptorSetRef &descriptorSet, int binding);

    class VulkanRHI *rhi;

    vkr::CommandBuffer commandBuffer;

    std::vector<RHIResourceRef> ownedResources;

    std::vector<VulkanDescriptorSetRef> currentDescriptorSets;

    RHIGraphicsPipelineRef currentPipeline;
};

const int MAX_FRAMES_IN_FLIGHT = 2;

class VulkanRHI : public RHIDriver {

public:
    VulkanRHI();

    virtual RHIShaderRef CreateShader(const std::vector<uint32_t> &code, ShaderType type) override;

    virtual RHIBufferRef CreateBuffer(size_t size, const BufferInfo &info) override;

    virtual RHIGraphicsPipelineRef CreateGraphicsPipeline(const GraphicsPipelineCreateInfo &info) override;

    virtual RHICommandListRef CreateCommandList() override;

    virtual void Submit(RHICommandListRef &commandList) override;

    virtual void SubmitAndWaitIdle(RHICommandListRef &commandList) override;

    virtual RHIRenderTargetRef BeginFrame() override;

    virtual RHITextureRef CreateTexture(uint32_t width, uint32_t height) override;

    virtual RHISamplerRef CreateSampler() override;

    virtual void CopyBufferToTexture(const RHIBufferRef &buf, RHITextureRef &texture, uint32_t width, uint32_t height) override;

    virtual void EndFrame() override;

    vk::RenderPass createRenderPass(int imgIdx);

    vk::Framebuffer createFramebuffer(const vk::RenderPass &renderPass, const VulkanRenderTarget &renderTarget);

    vk::Extent2D getExtent();

    VulkanDescriptorSetRef CreateDescriptorSet(const RHIGraphicsPipelineRef &pipeline, int num);


    void waitIdle();

private:
    VulkanWindow m_window;
    vkr::Context m_context;
    vkr::Instance m_instance;
    vkr::PhysicalDevice m_physicalDevice;
    vkr::SurfaceKHR m_surface;
    uint32_t m_graphicsFamilyIdx;
    uint32_t m_presentFamilyIdx;
    vkr::Device m_device;
    vkr::Queue m_graphicsQueue;
    vkr::Queue m_presentQueue;
    vk::SurfaceFormatKHR m_surfaceFormat;
    vk::PresentModeKHR m_presentMode;
    vk::SurfaceCapabilitiesKHR m_caps;
    vk::Extent2D m_extent;
    vkr::SwapchainKHR m_swapchain;
    std::vector<vk::raii::ImageView> m_swapchainImageViews;

    vkr::CommandPool m_commandPool;
    vkr::CommandBuffers m_commandBuffers;

    vkr::DescriptorPool m_descriptorPool;

    vkr::Semaphore m_imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
    vkr::Semaphore m_renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
    vkr::Fence m_inFlightFences[MAX_FRAMES_IN_FLIGHT];

    std::unordered_map<int, vkr::Framebuffer> m_frameBuffersCache;
    std::unordered_map<int, vkr::RenderPass> m_renderPassCache;

    uint32_t m_currentSwapchainImgIdx;
    uint32_t m_currentFrame;

    VulkanImageRef m_depthImage;

    std::unordered_multimap<int, RHIResourceRef> m_inFlightResources;
};

#define VULKAN_RHI_SAFE_CALL(Result) do {if((Result) != VK_SUCCESS) {}} while(0)
