#pragma once
#define SDL_MAIN_HANDLED

#include <vulkan/vulkan_raii.hpp>
#include <SDL2/SDL.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <memory>
#include <map>
#include <unordered_map>
#include "RHI.h"

namespace vkr = vk::raii;

struct Window {
    SDL_Window *window;
    int width;
    int height;

    Window();

    ~Window();
};


struct RenderTarget {
    vk::ImageView swapchainImg;
    uint32_t imageIdx;
};

struct CommandList {
    explicit CommandList(vkr::CommandBuffer &&commandBuffer, class VulkanRHI *rhi);

    void begin();

    void end();

    void BindPipeline(const RHIGraphicsPipelineRef &pipeline);

    void beginRenderPass(const RenderTarget &renderTarget);

    void draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance);

    void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                     uint32_t firstInstance);

    void setViewport();

    void setScissors();

    void endRenderPass();

    void BindVertexBuffer(const RHIBufferRef &buf, int binding);

    void BindIndexBuffer(const RHIBufferRef &buf);

    void BindUniformBuffer(const std::string &name, const RHIBufferRef &buffer, int size);

    void BindTexture(const std::string &name, Image &texture, Sampler &sampler);

    void BindDescriptorSet(const RHIGraphicsPipelineRef &pipeline, const RHIDescriptorSetRef &descriptorSet, int binding);

    class VulkanRHI *rhi;

    vkr::CommandBuffer commandBuffer;

    std::vector<RHIResourceRef> ownedResources;

    std::vector<RHIDescriptorSetRef> currentDescriptorSets;

    RHIGraphicsPipelineRef currentPipeline;
};

const int MAX_FRAMES_IN_FLIGHT = 2;

class VulkanRHI {

public:
    VulkanRHI();

    Shader createShader(const std::vector<uint32_t> &code, ShaderType type);

    RHIBufferRef CreateBuffer(size_t size, const BufferInfo &info);

    RHIGraphicsPipelineRef CreateGraphicsPipeline(const GraphicsPipelineCreateInfo &info);

    CommandList createCommandList();

    void submit(CommandList &commandList);

    void submitAndWaitIdle(CommandList &commandList);

    vk::RenderPass createRenderPass(int imgIdx);

    vk::Framebuffer createFramebuffer(const vk::RenderPass &renderPass, const RenderTarget &renderTarget);

    vk::Extent2D getExtent();

    RenderTarget beginFrame();

    RHIDescriptorSetRef CreateDescriptorSet(const RHIGraphicsPipelineRef &pipeline, int num);

    Image createImage(uint32_t width, uint32_t height);

    Sampler createSampler();

    void CopyBufferToImage(const RHIBufferRef &buf, Image &image, uint32_t width, uint32_t height);

    void endFrame();

    void waitIdle();

private:
    Window m_window;
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

    Image m_depthImage;

    std::unordered_multimap<int, CommandList> m_inFlightCommandLists;
};
