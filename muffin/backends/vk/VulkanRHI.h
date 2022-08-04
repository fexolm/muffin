#pragma once
#define SDL_MAIN_HANDLED

#include <vulkan/vulkan_raii.hpp>
#include <SDL2/SDL.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <memory>
#include <map>

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

    std::map<int, std::vector<vk::DescriptorSetLayoutBinding>> bindings;
    std::vector<vk::VertexInputAttributeDescription> vertexAttributes;
    std::vector<vk::VertexInputBindingDescription> vertexBindings;
};

enum VertexElementType {
    None,
    Float1,
    Float2,
    Float3,
    Float4,
    PackedNormal,    // FPackedNormal
    UByte4,
    UByte4N,
    Color,
    Short2,
    Short4,
    Short2N,        // 16 bit word normalized to (value/32767.0,value/32767.0,0,0,1)
    Half2,            // 16 bit float using 1 bit sign, 5 bit exponent, 10 bit mantissa
    Half4,
    Short4N,        // 4 X 16 bit word, normalized
    UShort2,
    UShort4,
    UShort2N,        // 16 bit word normalized to (value/65535.0,value/65535.0,0,0,1)
    UShort4N,        // 4 X 16 bit word unsigned, normalized
    URGB10A2N,        // 10 bit r, g, b and 2 bit a normalized to (value/1023.0f, value/1023.0f, value/1023.0f, value/3.0f)
    UInt,
    MAX,

    NumBits = 5,
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
    uint32_t imageIdx;
};

struct GraphicsPipeline {
    GraphicsPipeline(vkr::Pipeline &&pipeline, vkr::PipelineLayout &&layout,
                     std::vector<vkr::DescriptorSetLayout> &&descriptorSetLayouts);

    vkr::PipelineLayout layout;
    vkr::Pipeline pipeline;
    std::vector<vkr::DescriptorSetLayout> descriptorSetLayouts;
};

struct Image {
    vkr::Image image;
    vkr::DeviceMemory memory;
    vkr::ImageView view;
};

struct Sampler {
    vkr::Sampler sampler;
};

struct DescriptorSet {
    vkr::DescriptorSet descriptorSet;
    vkr::Device *device;

    void update(int binding, Buffer &buffer, int size);
    void update(int binding, Image &image, Sampler &sampler);
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

    void bindVertexBuffer(const Buffer &buf, int binding);

    void bindIndexBuffer(const Buffer &buf);

    void bindDescriptorSet(const GraphicsPipeline &pipeline, const DescriptorSet &set, int binding);

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

    vk::RenderPass createRenderPass(int imgIdx);

    vk::Framebuffer createFramebuffer(const vk::RenderPass &renderPass, const RenderTarget &renderTarget);

    vk::Extent2D getExtent();

    RenderTarget beginFrame();

    DescriptorSet createDescriptorSet(const GraphicsPipeline &pipeline, int num);

    Image createImage(uint32_t width, uint32_t height);

    Sampler createSampler();

    void copyBuffer(const Buffer &srcBuffer, Buffer &dstBuffer, int size);

    void copyBufferToImage(const Buffer &buf, Image &image, uint32_t width, uint32_t height);

    void updateDescriptorSet(DescriptorSet &set, Buffer &buffer, Image &image, Sampler &sampler, int size);

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

    vkr::Semaphore m_imageAvailableSemaphore;
    vkr::Semaphore m_renderFinishedSemaphore;
    vkr::Fence m_inFlightFence;

    std::unordered_map<int, vkr::Framebuffer> m_frameBuffersCache;
    std::unordered_map<int, vkr::RenderPass> m_renderPassCache;

    uint32_t m_currentSwapchainImgIdx;

    Image m_depthImage;
};
