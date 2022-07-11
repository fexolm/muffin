#pragma once
#define SDL_MAIN_HANDLED

#include <vulkan/vulkan_raii.hpp>
#include <SDL2/SDL.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <memory>

namespace vkr = vk::raii;


struct Window {
    SDL_Window *window;
    int width;
    int height;

    Window();

    ~Window();
};

struct Vertex {
    glm::vec2 pos;
    glm::vec3 color;
};

struct Shader {
    explicit Shader(vkr::ShaderModule &&module);

    vkr::ShaderModule module;
};

struct Buffer {
    vkr::Buffer buffer;
    vkr::DeviceMemory memory;

    void fill(void *data, size_t size);
};

struct GraphicsPipelineCreateInfo {
    std::shared_ptr<Shader> vertexShader;
    std::shared_ptr<Shader> fragmentShader;
};

struct RenderTarget {
    vk::ImageView swapchainImg;
};

struct GraphicsPipeline {
    GraphicsPipeline(vkr::Pipeline &&pipeline, vkr::PipelineLayout &&layout);

    vkr::PipelineLayout layout;
    vkr::Pipeline pipeline;
};

struct CommandList {
    explicit CommandList(vkr::CommandBuffer &&commandBuffer, class VulkanRHI *rhi);

    void begin();

    void end();

    void bindPipeline(const GraphicsPipeline &pipeline);

    void beginRenderPass(const RenderTarget &renderTarget);

    void draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance);

    void setViewport();

    void setScissors();

    void endRenderPass();

    void bindVertexBuffer(const Buffer &buf);

    class VulkanRHI *rhi;

    vkr::CommandBuffer commandBuffer;
};

class VulkanRHI {

public:
    VulkanRHI();

    Shader createShader(const std::vector<uint32_t> &code);

    Buffer createBuffer(size_t size);

    GraphicsPipeline createGraphicsPipeline(const GraphicsPipelineCreateInfo &info);

    CommandList createCommandList();

    void submit(CommandList &commandList);

    vk::RenderPass createRenderPass();

    vk::Framebuffer createFramebuffer(const vk::RenderPass &renderPass, const RenderTarget &renderTarget);

    vk::Extent2D getExtent();

    RenderTarget beginFrame();

    void endFrame();

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

    vkr::Semaphore m_imageAvailableSemaphore;
    vkr::Semaphore m_renderFinishedSemaphore;
    vkr::Fence m_inFlightFence;

    std::vector<vkr::Framebuffer> m_frameBuffersCache;
    std::vector<vkr::RenderPass> m_renderPassCache;

    uint32_t m_currentSwapchainImgIdx;
};
