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

class VulkanRHI {

public:
    VulkanRHI();

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
    vkr::SwapchainKHR m_swapchain;
    std::vector<vk::raii::ImageView> m_swapchainImageViews;
    vkr::CommandPool m_commandPool;
    vkr::CommandBuffers m_commandBuffers;
    vkr::DeviceMemory m_depthDeviceMemory;
    vkr::Image m_depthImage;
    vkr::ImageView m_depthImageView;
};
