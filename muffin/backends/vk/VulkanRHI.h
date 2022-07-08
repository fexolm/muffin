#pragma once
#define SDL_MAIN_HANDLED

#include <vulkan/vulkan_raii.hpp>
#include <SDL2/SDL.h>
#include <memory>

namespace vkr = vk::raii;


struct Window {
    SDL_Window *window;
    int width;
    int height;

    Window();

    ~Window();
};

struct Shader {
    Shader(vkr::ShaderModule &&module);

    vkr::ShaderModule module;
};

struct VertexInputState {
    vk::PipelineVertexInputStateCreateInfo createInfo;
};

struct GraphicsPipelineCreateInfo {
    std::shared_ptr<Shader> vertexShader;
    std::shared_ptr<Shader> fragmentShader;
};

struct GraphicsPipeline {
    GraphicsPipeline(vkr::Pipeline &&pipeline);

    vkr::Pipeline pipeline;
};

class VulkanRHI {

public:
    VulkanRHI();

    Shader createShader(const std::vector<char> &code);

    GraphicsPipeline createGraphicsPipeline(const GraphicsPipelineCreateInfo &info);

    void drawTriangle(const GraphicsPipeline &graphicsPipeline);

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
    vkr::PipelineLayout m_pipelineLayout;
    vkr::RenderPass m_renderPass;
    std::vector<vkr::Framebuffer> m_framebuffers;
    vkr::CommandPool m_commandPool;
    vkr::CommandBuffers m_commandBuffers;
};
