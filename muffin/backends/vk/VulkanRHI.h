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
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
};

enum class ShaderType {
    Vertex,
    Fragment
};

struct Shader {
    explicit Shader(vkr::ShaderModule &&module);

    vkr::ShaderModule module;

    std::vector<vk::DescriptorSetLayoutBinding> bindings;
};

enum class BufferUsage {
    Vertex,
    Index,
    Uniform,
    Staging,
};

struct BufferInfo {
    BufferUsage usage;
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
    GraphicsPipeline(vkr::Pipeline &&pipeline, vkr::PipelineLayout &&layout,
                     vkr::DescriptorSetLayout &&descriptorSetLayout);

    vkr::PipelineLayout layout;
    vkr::Pipeline pipeline;
    vkr::DescriptorSetLayout descriptorSetLayout;
};

struct DescriptorSet {
    vkr::DescriptorSet descriptorSet;
};

struct Texture {
    vkr::Image image;
    vkr::DeviceMemory memory;
    vkr::ImageView view;
};

struct Sampler {
    vkr::Sampler sampler;
};

struct CommandList {
    explicit CommandList(vkr::CommandBuffer &&commandBuffer, class VulkanRHI *rhi);

    void begin();

    void end();

    void bindPipeline(const GraphicsPipeline &pipeline);

    void beginRenderPass(const RenderTarget &renderTarget);

    void draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance);

    void drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                     uint32_t firstInstance);

    void setViewport();

    void setScissors();

    void endRenderPass();

    void bindVertexBuffer(const Buffer &buf);

    void bindIndexBuffer(const Buffer &buf);

    void bindDescriptorSet(const GraphicsPipeline &pipeline, const DescriptorSet &set);

    class VulkanRHI *rhi;

    vkr::CommandBuffer commandBuffer;
};

class VulkanRHI {

public:
    VulkanRHI();

    Shader createShader(const std::vector<uint32_t> &code, ShaderType type);

    Buffer createBuffer(size_t size, const BufferInfo &info);

    GraphicsPipeline createGraphicsPipeline(const GraphicsPipelineCreateInfo &info);

    CommandList createCommandList();

    void submit(CommandList &commandList);

    void submitAndWaitIdle(CommandList &commandList);

    vk::RenderPass createRenderPass();

    vk::Framebuffer createFramebuffer(const vk::RenderPass &renderPass, const RenderTarget &renderTarget);

    vk::Extent2D getExtent();

    RenderTarget beginFrame();

    DescriptorSet createDescriptorSet(const GraphicsPipeline &pipeline);

    Texture createTexture(uint32_t width, uint32_t height);

    Sampler createSampler();

    void copyBuffer(const Buffer &srcBuffer, Buffer &dstBuffer, int size);

    void copyBufferToTexture(const Buffer &buf, Texture &texture, uint32_t width, uint32_t height);

    void updateDescriptorSet(DescriptorSet &set, Buffer &buffer, Texture &texture, Sampler &sampler, int size);

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

    vkr::DescriptorPool m_descriptorPool;

    vkr::Semaphore m_imageAvailableSemaphore;
    vkr::Semaphore m_renderFinishedSemaphore;
    vkr::Fence m_inFlightFence;

    std::vector<vkr::Framebuffer> m_frameBuffersCache;
    std::vector<vkr::RenderPass> m_renderPassCache;

    uint32_t m_currentSwapchainImgIdx;

    Texture m_depthImage;
};
