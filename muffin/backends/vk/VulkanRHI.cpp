#include "VulkanRHI.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <limits>

vkr::Instance createInstance(const vkr::Context &context, const std::vector<const char *> &enabledExtensions) {
    vk::ApplicationInfo applicationInfo;
    applicationInfo.pApplicationName = "Muffin";
    applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.pEngineName = "Muffin";
    applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    applicationInfo.apiVersion = VK_API_VERSION_1_0;

    // TODO: check extensions support
    const std::vector<const char *> validationLayers = {
            "VK_LAYER_KHRONOS_validation"
    };

    vk::InstanceCreateInfo instanceCreateInfo;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
    instanceCreateInfo.enabledExtensionCount = (uint32_t) enabledExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
    instanceCreateInfo.enabledLayerCount = (uint32_t) validationLayers.size();
    instanceCreateInfo.ppEnabledLayerNames = validationLayers.data();

    vkr::Instance instance(context, instanceCreateInfo);
    return instance;
}

uint32_t findGraphicsFamilyIdx(const vkr::PhysicalDevice &device) {
    auto families = device.getQueueFamilyProperties();
    for (uint32_t i = 0; i < families.size(); ++i) {
        if (families[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            return i;
        }
    }
    return uint32_t(-1);
}

uint32_t findPresentFamilyIdx(const vkr::PhysicalDevice &device, const vkr::SurfaceKHR &surface) {
    auto families = device.getQueueFamilyProperties();
    for (uint32_t i = 0; i < families.size(); ++i) {
        if (families[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            if (device.getSurfaceSupportKHR(i, *surface)) {
                return i;
            }
        }
    }
    return uint32_t(-1);
}

vkr::PhysicalDevice choosePhysicalDevice(const vkr::Instance &instance) {
    vkr::PhysicalDevices physicalDevices(instance);

    for (auto &device: physicalDevices) {
        auto props = device.getProperties();
        if (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            return device;
        }
    }
    // TODO: throw
    return nullptr;
}

vkr::SurfaceKHR createSurface(const vkr::Instance &instance, SDL_Window *window) {
    VkSurfaceKHR surfaceHandle;
    if (!SDL_Vulkan_CreateSurface(window, *instance, (SDL_vulkanSurface *) &surfaceHandle)) {
        throw;
    }
    vkr::SurfaceKHR surface(instance, surfaceHandle);
    return surface;
}

vkr::Device
createDevice(const vkr::PhysicalDevice &physicalDevice, uint32_t graphicsFamilyIdx, uint32_t presentFamilyIdx,
             const std::vector<const char *> &deviceExtensions) {
    float queuePriority = 1.0f;

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.emplace_back();
    queueCreateInfos.back().queueFamilyIndex = graphicsFamilyIdx;
    queueCreateInfos.back().queueCount = 1;
    queueCreateInfos.back().pQueuePriorities = &queuePriority;

    if (graphicsFamilyIdx != presentFamilyIdx) {
        queueCreateInfos.emplace_back();
        queueCreateInfos.back().queueFamilyIndex = presentFamilyIdx;
        queueCreateInfos.back().queueCount = 1;
        queueCreateInfos.back().pQueuePriorities = &queuePriority;
    }

    vk::PhysicalDeviceFeatures deviceFeatures;
    deviceFeatures.samplerAnisotropy = true;


    vk::DeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = (uint32_t) queueCreateInfos.size();
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.enabledExtensionCount = (uint32_t) deviceExtensions.size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    vkr::Device device(physicalDevice, deviceCreateInfo);

    return device;
}

vkr::CommandPool createCommandPool(const vkr::Device &device) {
    vk::CommandPoolCreateInfo commandPoolCreateInfo;
    vkr::CommandPool commandPool(device, commandPoolCreateInfo);
    return commandPool;
}

vkr::CommandBuffers createCommandBuffers(const vkr::Device &device, const vkr::CommandPool &commandPool) {
    vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
    commandBufferAllocateInfo.commandPool = *commandPool;
    vkr::CommandBuffers commandBuffers(device, commandBufferAllocateInfo);
    return commandBuffers;
}

vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats) {
    for (const auto &availableFormat: availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes) {
    for (const auto &availablePresentMode: availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        }
    }

    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D
chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities, int width, int height) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {

        vk::Extent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                                        capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                                         capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

vkr::SwapchainKHR
createSwapchain(const vkr::SurfaceKHR &surface, const vkr::Device &device,
                const vk::SurfaceFormatKHR &surfaceFormat, const vk::PresentModeKHR &presentMode,
                const vk::SurfaceCapabilitiesKHR &caps,
                const vk::Extent2D &extent, uint32_t graphicsFamilyIdx, uint32_t presentFamilyIdx) {
    uint32_t imageCount = caps.minImageCount + 1;

    vk::SwapchainCreateInfoKHR swapchainCreateInfo;
    swapchainCreateInfo.surface = *surface;
    swapchainCreateInfo.minImageCount = imageCount;
    swapchainCreateInfo.imageFormat = surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = extent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

    uint32_t queueFamilyIndices[] = {graphicsFamilyIdx, presentFamilyIdx};
    if (graphicsFamilyIdx == presentFamilyIdx) {
        swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
        swapchainCreateInfo.queueFamilyIndexCount = 0;
        swapchainCreateInfo.pQueueFamilyIndices = nullptr;
    } else {
        swapchainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }

    swapchainCreateInfo.preTransform = caps.currentTransform;
    swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = true;
    swapchainCreateInfo.oldSwapchain = nullptr;

    vkr::SwapchainKHR swapchain(device, swapchainCreateInfo);
    return swapchain;
}

std::vector<vk::raii::ImageView>
createSwapchainImageViews(const vkr::SwapchainKHR &swapchain, const vkr::Device &device,
                          const vk::SurfaceFormatKHR &surfaceFormat, const vk::Extent2D &extent) {
    std::vector<vk::raii::ImageView> imageViews;

    auto images = swapchain.getImages();
    for (auto image: images) {
        vk::ImageViewCreateInfo imageViewCreateInfo;
        imageViewCreateInfo.image = image;
        imageViewCreateInfo.format = surfaceFormat.format;
        imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
        imageViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
        imageViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
        imageViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
        imageViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
        imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = 1;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;

        imageViews.emplace_back(device, imageViewCreateInfo);
    }
    return imageViews;
}


VulkanRHI::VulkanRHI() :
        m_context(),
        m_instance(nullptr),
        m_physicalDevice(nullptr),
        m_surface(nullptr),
        m_device(nullptr),
        m_graphicsQueue(nullptr),
        m_presentQueue(nullptr),
        m_commandPool(nullptr),
        m_commandBuffers(nullptr),
        m_swapchain(nullptr),
        m_swapchainImageViews(),
        m_depthDeviceMemory(nullptr),
        m_depthImage(nullptr),
        m_depthImageView(nullptr) {


    uint32_t extensionsCount;
    SDL_Vulkan_GetInstanceExtensions(m_window.window, &extensionsCount, nullptr);
    std::vector<const char *> enabledExtensions(extensionsCount);
    SDL_Vulkan_GetInstanceExtensions(m_window.window, &extensionsCount, enabledExtensions.data());

    const std::vector<const char *> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    m_instance = createInstance(m_context, enabledExtensions);
    m_physicalDevice = choosePhysicalDevice(m_instance);

    m_surface = createSurface(m_instance, m_window.window);
    m_graphicsFamilyIdx = findGraphicsFamilyIdx(m_physicalDevice);
    m_presentFamilyIdx = findPresentFamilyIdx(m_physicalDevice, m_surface);
    m_device = createDevice(m_physicalDevice, m_graphicsFamilyIdx, m_presentFamilyIdx, deviceExtensions);
    m_graphicsQueue = m_device.getQueue(m_graphicsFamilyIdx, 0);
    m_presentQueue = m_device.getQueue(m_presentFamilyIdx, 0);


    vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(m_physicalDevice.getSurfaceFormatsKHR(*m_surface));
    vk::PresentModeKHR presentMode = chooseSwapPresentMode(m_physicalDevice.getSurfacePresentModesKHR(*m_surface));
    vk::SurfaceCapabilitiesKHR caps = m_physicalDevice.getSurfaceCapabilitiesKHR(*m_surface);
    vk::Extent2D extent = chooseSwapExtent(caps, m_window.width, m_window.height);

    m_swapchain = createSwapchain(m_surface, m_device, surfaceFormat, presentMode, caps, extent, m_graphicsFamilyIdx,
                                  m_presentFamilyIdx);

    m_swapchainImageViews = createSwapchainImageViews(m_swapchain, m_device, surfaceFormat, extent);


//    m_commandPool = createCommandPool(m_device);
//    m_commandBuffers = createCommandBuffers(m_device, m_commandPool);
}

Window::Window() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    window = SDL_CreateWindow("SDL Vulkan Sample", 0, 0, 800, 600, SDL_WINDOW_VULKAN);
    SDL_GetWindowSize(window, &width, &height);
}

Window::~Window() {
    SDL_DestroyWindow(window);
    SDL_Quit();
}
