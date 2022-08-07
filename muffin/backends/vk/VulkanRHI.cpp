#include "VulkanRHI.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <spirv_cross/spirv_cross.hpp>

#include <limits>

#include "VulkanBuffer.h"
#include "VulkanDescriptorSet.h"
#include "VulkanGraphicsPipeline.h"

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

uint32_t
findMemoryType(const vkr::PhysicalDevice &physicalDevice, uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    auto deviceMemProps = physicalDevice.getMemoryProperties();
    for (uint32_t i = 0; i < deviceMemProps.memoryTypeCount; i++) {
        if (typeFilter & (1 << i) && (deviceMemProps.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
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

vkr::CommandPool createCommandPool(const vkr::Device &device, uint32_t graphicsFamilyIdx) {
    vk::CommandPoolCreateInfo commandPoolCreateInfo;
    commandPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    commandPoolCreateInfo.queueFamilyIndex = graphicsFamilyIdx;
    vkr::CommandPool commandPool(device, commandPoolCreateInfo);
    return commandPool;
}

vkr::CommandBuffers createCommandBuffers(const vkr::Device &device, const vkr::CommandPool &commandPool) {
    vk::CommandBufferAllocateInfo commandBufferAllocateInfo;
    commandBufferAllocateInfo.commandPool = *commandPool;
    commandBufferAllocateInfo.level = vk::CommandBufferLevel::ePrimary;
    commandBufferAllocateInfo.commandBufferCount = 1;
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
chooseSwapExtent(const vk::SurfaceCapabilitiesKHR &capabilities, uint32_t width, uint32_t height) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {

        vk::Extent2D actualExtent = {width, height};

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

vkr::DescriptorPool createDescriptorPool(const vkr::Device &device) {
    vk::DescriptorPoolSize poolSizes[2];
    poolSizes[0].descriptorCount = 4096;
    poolSizes[0].type = vk::DescriptorType::eUniformBuffer;

    poolSizes[1].descriptorCount = 4096;
    poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;

    vk::DescriptorPoolCreateInfo createInfo;
    createInfo.poolSizeCount = 2;
    createInfo.pPoolSizes = poolSizes;
    createInfo.maxSets = 1000;
    createInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;

    vkr::DescriptorPool descriptorPool(device, createInfo);
    return descriptorPool;
}

Image
createImageImpl(const vkr::Device &device, const vkr::PhysicalDevice &physicalDevice, uint32_t width, uint32_t height,
                vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                vk::MemoryPropertyFlags memoryPorperties, vk::ImageAspectFlagBits aspectMask) {
    vk::ImageCreateInfo imageInfo;
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.usage = usage;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.samples = vk::SampleCountFlagBits::e1;

    vkr::Image image = device.createImage(imageInfo);

    vk::MemoryRequirements memReq = image.getMemoryRequirements();

    vk::MemoryAllocateInfo allocInfo;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memReq.memoryTypeBits, memoryPorperties);

    vkr::DeviceMemory memory = device.allocateMemory(allocInfo);

    image.bindMemory(*memory, 0);

    vk::ImageViewCreateInfo viewInfo;
    viewInfo.image = *image;
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectMask;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    vkr::ImageView view = device.createImageView(viewInfo);

    return Image{std::move(image), std::move(memory), std::move(view)};
}

vkr::DescriptorSetLayout
createDescriptorSetLayout(const vkr::Device &device, const std::vector<vk::DescriptorSetLayoutBinding> &bindings) {
    vk::DescriptorSetLayoutCreateInfo createInfo;
    createInfo.bindingCount = bindings.size();
    createInfo.pBindings = bindings.data();

    vkr::DescriptorSetLayout descriptorSetLayout(device, createInfo);
    return descriptorSetLayout;
}

vkr::PipelineLayout
createPipelineLayout(const vkr::Device &device, const std::vector<vk::DescriptorSetLayout> &descriptorSetLayouts) {
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
    pipelineLayoutInfo.setLayoutCount = descriptorSetLayouts.size();
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    vkr::PipelineLayout pipelineLayout(device, pipelineLayoutInfo);
    return pipelineLayout;
}

bool hasStencilComponent(vk::Format format) {
    return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

void transitionImageLayout(VulkanRHI &rhi, const vkr::Image &img, vk::Format format, vk::ImageLayout oldLayout,
                           vk::ImageLayout newLayout) {
    auto cmdList = rhi.createCommandList();
    cmdList.begin();

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destStage;

    vk::ImageMemoryBarrier barrier;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = *img;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;

        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
        }
    } else {
        barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    }

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eNone;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
               newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else if (oldLayout == vk::ImageLayout::eUndefined &&
               newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eNone;
        barrier.dstAccessMask =
                vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    cmdList.commandBuffer.pipelineBarrier(sourceStage, destStage,
                                          (vk::DependencyFlagBits) 0, {}, {}, barrier);

    cmdList.end();
    rhi.submitAndWaitIdle(cmdList);
}

vk::Format findSupportedFormat(const vkr::PhysicalDevice &physicalDevice, const std::vector<vk::Format> &candidates,
                               vk::ImageTiling tiling,
                               vk::FormatFeatureFlagBits features) {
    for (vk::Format format: candidates) {
        vk::FormatProperties props = physicalDevice.getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format!");
}

vk::Format findDepthFormat(const vkr::PhysicalDevice &physicalDevice) {
    return findSupportedFormat(physicalDevice,
                               {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
                               vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

Image
createDepthImage(VulkanRHI &rhi, const vkr::Device &device, const vkr::PhysicalDevice &physicalDevice, uint32_t width,
                 uint32_t height) {
    vk::Format depthFormat = findDepthFormat(physicalDevice);
    Image image = createImageImpl(device, physicalDevice, width, height, depthFormat, vk::ImageTiling::eOptimal,
                                  vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                  vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eDepth);

    transitionImageLayout(rhi, image.image, depthFormat, vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eDepthStencilAttachmentOptimal);
    return image;
}

vk::RenderPass VulkanRHI::createRenderPass(int imgIdx) {
    if(m_renderPassCache.count(imgIdx)) {
        return *m_renderPassCache.at(imgIdx);
    }

    vk::AttachmentDescription colorAttachment;
    colorAttachment.format = m_surfaceFormat.format;
    colorAttachment.samples = vk::SampleCountFlagBits::e1;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
    colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference colorAttachmentRef;
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::AttachmentDescription depthAttachment;
    depthAttachment.format = findDepthFormat(m_physicalDevice);
    depthAttachment.samples = vk::SampleCountFlagBits::e1;
    depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
    depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::AttachmentReference depthAttachmentRef;
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    vk::SubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask =
            vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.srcAccessMask = vk::AccessFlagBits::eNone;

    dependency.dstStageMask =
            vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
    dependency.dstAccessMask =
            vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

    std::array<vk::AttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
    vk::RenderPassCreateInfo renderPassInfo;
    renderPassInfo.attachmentCount = attachments.size();
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    m_renderPassCache.emplace(imgIdx, vkr::RenderPass(m_device, renderPassInfo));

    return *m_renderPassCache.at(imgIdx);
}

RHIGraphicsPipelineRef VulkanRHI::CreateGraphicsPipeline(const GraphicsPipelineCreateInfo &info) {
    return RHIGraphicsPipelineRef(new VulkanGraphicsPipeline(*m_device, (VkExtent2D)m_extent, (VkFormat)m_surfaceFormat.format, (VkFormat)findDepthFormat(m_physicalDevice), info));
}

vk::Framebuffer
VulkanRHI::createFramebuffer(const vk::RenderPass &renderPass, const RenderTarget &renderTarget) {
    if(m_frameBuffersCache.count(renderTarget.imageIdx)) {
        return *m_frameBuffersCache.at(renderTarget.imageIdx);
    }

    vk::ImageView attachments[] = {
            renderTarget.swapchainImg, *m_depthImage.view
    };
    vk::FramebufferCreateInfo framebufferInfo;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 2;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = m_extent.width;
    framebufferInfo.height = m_extent.height;
    framebufferInfo.layers = 1;

    m_frameBuffersCache.emplace(renderTarget.imageIdx, vkr::Framebuffer(m_device, framebufferInfo));
    return *m_frameBuffersCache.at(renderTarget.imageIdx);
}

VulkanRHI::VulkanRHI() :
        m_context(),
        m_instance(nullptr),
        m_physicalDevice(nullptr),
        m_surface(nullptr),
        m_device(nullptr),
        m_graphicsQueue(nullptr),
        m_presentQueue(nullptr),
        m_swapchain(nullptr),
        m_swapchainImageViews(),
        m_commandPool(nullptr),
        m_commandBuffers(nullptr),
        m_descriptorPool(nullptr),
        m_imageAvailableSemaphores{nullptr, nullptr},
        m_renderFinishedSemaphores{nullptr, nullptr},
        m_inFlightFences{nullptr, nullptr},
        m_depthImage{nullptr, nullptr, nullptr} {

    uint32_t extensionsCount;
    SDL_Vulkan_GetInstanceExtensions(m_window.window, &extensionsCount, nullptr);
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

    m_surfaceFormat = chooseSwapSurfaceFormat(m_physicalDevice.getSurfaceFormatsKHR(*m_surface));
    m_presentMode = chooseSwapPresentMode(m_physicalDevice.getSurfacePresentModesKHR(*m_surface));
    m_caps = m_physicalDevice.getSurfaceCapabilitiesKHR(*m_surface);
    m_extent = chooseSwapExtent(m_caps, m_window.width, m_window.height);

    m_swapchain = createSwapchain(m_surface, m_device, m_surfaceFormat, m_presentMode, m_caps, m_extent,
                                  m_graphicsFamilyIdx,
                                  m_presentFamilyIdx);

    m_swapchainImageViews = createSwapchainImageViews(m_swapchain, m_device, m_surfaceFormat, m_extent);

    m_commandPool = createCommandPool(m_device, m_graphicsFamilyIdx);

    m_descriptorPool = createDescriptorPool(m_device);

    vk::SemaphoreCreateInfo semaphoreInfo;

    vk::FenceCreateInfo fenceInfo;
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    for(int i=0; i<MAX_FRAMES_IN_FLIGHT; i++) {
        m_imageAvailableSemaphores[i] = vkr::Semaphore(m_device, semaphoreInfo);
        m_renderFinishedSemaphores[i] = vkr::Semaphore(m_device, semaphoreInfo);
        m_inFlightFences[i] = vkr::Fence(m_device, fenceInfo);
    }
    m_currentFrame = 0;

    m_depthImage = createDepthImage(*this, m_device, m_physicalDevice, m_extent.width, m_extent.height);
}

#include <iostream>

static inline VkFormat toVkBufferFormat(VertexElementType Type) {
    switch (Type) {
        case Float1:
            return VK_FORMAT_R32_SFLOAT;
        case Float2:
            return VK_FORMAT_R32G32_SFLOAT;
        case Float3:
            return VK_FORMAT_R32G32B32_SFLOAT;
        case PackedNormal:
            return VK_FORMAT_R8G8B8A8_SNORM;
        case UByte4:
            return VK_FORMAT_R8G8B8A8_UINT;
        case UByte4N:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case Color:
            return VK_FORMAT_B8G8R8A8_UNORM;
        case Short2:
            return VK_FORMAT_R16G16_SINT;
        case Short4:
            return VK_FORMAT_R16G16B16A16_SINT;
        case Short2N:
            return VK_FORMAT_R16G16_SNORM;
        case Half2:
            return VK_FORMAT_R16G16_SFLOAT;
        case Half4:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        case Short4N:        // 4 X 16 bit word: normalized
            return VK_FORMAT_R16G16B16A16_SNORM;
        case UShort2:
            return VK_FORMAT_R16G16_UINT;
        case UShort4:
            return VK_FORMAT_R16G16B16A16_UINT;
        case UShort2N:        // 16 bit word normalized to (value/65535.0:value/65535.0:0:0:1)
            return VK_FORMAT_R16G16_UNORM;
        case UShort4N:        // 4 X 16 bit word unsigned: normalized
            return VK_FORMAT_R16G16B16A16_UNORM;
        case Float4:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case URGB10A2N:
            return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
        case UInt:
            return VK_FORMAT_R32_UINT;
        default:
            break;
    }

    throw std::runtime_error("Undefined vertex-element format conversion");
}

uint32_t getTypeSize(VertexElementType type) {
    switch (type) {
        case VertexElementType::Float1:
            return 4;
        case VertexElementType::Float2:
            return 8;
        case VertexElementType::Float3:
            return 12;
        case VertexElementType::Float4:
            return 16;
        default:
            throw std::runtime_error("Unsuported type");
    }
}

VertexElementType spirvToVertexElementType(spirv_cross::SPIRType type) {
    if (type.basetype == spirv_cross::SPIRType::Float) {
        if (type.vecsize == 1) {
            return VertexElementType::Float1;
        }
        if (type.vecsize == 2) {
            return VertexElementType::Float2;
        }
        if (type.vecsize == 3) {
            return VertexElementType::Float3;
        }
        if (type.vecsize == 4) {
            return VertexElementType::Float4;
        }
    }
    return VertexElementType::None;
}

Shader VulkanRHI::createShader(const std::vector<uint32_t> &code, ShaderType type) {
    vk::ShaderModuleCreateInfo createInfo;
    createInfo.codeSize = code.size() * sizeof(uint32_t);
    createInfo.pCode = code.data();
    vkr::ShaderModule shaderMoule(m_device, createInfo);

    spirv_cross::Compiler comp(code);

    auto resources = comp.get_shader_resources();

    auto res = Shader{std::move(shaderMoule)};

    if (type == ShaderType::Vertex) {
        int i = 0;
        for (auto &b: resources.stage_inputs) {
            VkVertexInputAttributeDescription attributeDescription;
            attributeDescription.binding = i;
            attributeDescription.location = comp.get_decoration(b.id, spv::DecorationLocation);
            attributeDescription.offset = 0;
            VertexElementType elementType = spirvToVertexElementType(comp.get_type(b.type_id));
            attributeDescription.format = toVkBufferFormat(elementType);

            VkVertexInputBindingDescription bindingDescription;
            bindingDescription.binding = i;
            bindingDescription.stride = getTypeSize(elementType);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            res.vertexAttributes.push_back(attributeDescription);
            res.vertexBindings.push_back(bindingDescription);

            i++;
        }
    }

    for (auto &ub: resources.uniform_buffers) {
        auto binding = comp.get_decoration(ub.id, spv::DecorationBinding);
        auto set = comp.get_decoration(ub.id, spv::DecorationDescriptorSet);

        VkDescriptorSetLayoutBinding layoutBinding;
        layoutBinding.binding = binding;
        layoutBinding.descriptorCount = 1;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        switch (type) {
            case ShaderType::Vertex:
                layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case ShaderType::Fragment:
                layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
        }
        layoutBinding.pImmutableSamplers = nullptr;

        res.bindings[set].push_back(layoutBinding);
    }

    for (auto &ub: resources.sampled_images) {
        auto binding = comp.get_decoration(ub.id, spv::DecorationBinding);
        auto set = comp.get_decoration(ub.id, spv::DecorationDescriptorSet);

        VkDescriptorSetLayoutBinding layoutBinding;
        layoutBinding.binding = binding;
        layoutBinding.descriptorCount = 1;
        layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

        switch (type) {
            case ShaderType::Vertex:
                layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            case ShaderType::Fragment:
                layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
        }
        layoutBinding.pImmutableSamplers = nullptr;
        res.bindings[set].push_back(layoutBinding);
    }

    return res;
}

void VulkanRHI::submit(CommandList &commandList) {
    vk::SubmitInfo submitInfo;
    vk::Semaphore waitSemaphores[] = {*m_imageAvailableSemaphores[m_currentFrame]};
    vk::CommandBuffer commandBuffers = {*commandList.commandBuffer};
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    vk::Semaphore signalSemaphores[] = {*m_renderFinishedSemaphores[m_currentFrame]};
    submitInfo.setWaitSemaphores(waitSemaphores)
            .setWaitDstStageMask(waitStages)
            .setCommandBuffers(commandBuffers)
            .setSignalSemaphores(signalSemaphores);

    m_graphicsQueue.submit({submitInfo}, *m_inFlightFences[m_currentFrame]);

    m_inFlightCommandLists.emplace(m_currentFrame, std::move(commandList));
}

CommandList VulkanRHI::createCommandList() {
    vkr::CommandBuffers commandBuffers = createCommandBuffers(m_device, m_commandPool);
    commandBuffers[0].reset();

    return CommandList(std::move(commandBuffers[0]), this);
}

vk::Extent2D VulkanRHI::getExtent() {
    return m_extent;
}

RenderTarget VulkanRHI::beginFrame() {
    m_device.waitForFences({*m_inFlightFences[m_currentFrame]}, true, UINT64_MAX);
    m_inFlightCommandLists.erase(m_currentFrame);
    m_device.resetFences({*m_inFlightFences[m_currentFrame]});
    m_currentSwapchainImgIdx = m_swapchain.acquireNextImage(UINT64_MAX, *m_imageAvailableSemaphores[m_currentFrame], nullptr).second;

    return RenderTarget{*m_swapchainImageViews[m_currentSwapchainImgIdx], m_currentSwapchainImgIdx};
}

void VulkanRHI::endFrame() {
    vk::Semaphore signalSemaphores[] = {*m_renderFinishedSemaphores[m_currentFrame]};
    vk::SwapchainKHR swapChains[] = {*m_swapchain};
    vk::PresentInfoKHR presentInfo;
    presentInfo.pSwapchains = swapChains;
    presentInfo.swapchainCount = 1;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.pResults = nullptr;

    presentInfo.pImageIndices = &m_currentSwapchainImgIdx;

    m_graphicsQueue.presentKHR(presentInfo);

    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

RHIBufferRef VulkanRHI::CreateBuffer(size_t size, const BufferInfo &info) {
    return RHIBufferRef(new VulkanBuffer(*m_device, *m_physicalDevice, size, info));
}

RHIDescriptorSetRef VulkanRHI::CreateDescriptorSet(const RHIGraphicsPipelineRef &pipeline, int num) {
    VulkanGraphicsPipeline *vulkanPipeline = static_cast<VulkanGraphicsPipeline *>(pipeline.get());
    VkDescriptorSetLayout layout = vulkanPipeline->DescriptorLayouts()[num];
    return RHIDescriptorSetRef(new VulkanDescriptorSet(*m_device, *m_descriptorPool, &layout));
}

Image VulkanRHI::createImage(uint32_t width, uint32_t height) {
    return createImageImpl(m_device, m_physicalDevice, width, height, vk::Format::eR8G8B8A8Srgb,
                           vk::ImageTiling::eOptimal,
                           vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                           vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor);
}

void VulkanRHI::CopyBufferToImage(const RHIBufferRef &buf, Image &image, uint32_t width, uint32_t height) {
    VulkanBuffer *buffer = static_cast<VulkanBuffer *>(buf.get());
    transitionImageLayout(*this, image.image, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eTransferDstOptimal);

    auto cmdList = createCommandList();
    cmdList.begin();

    vk::BufferImageCopy region;
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = vk::Offset3D{0, 0, 0};
    region.imageExtent = vk::Extent3D{width, height, 1};

    cmdList.commandBuffer.copyBufferToImage(buffer->BufferHandle(), *image.image, vk::ImageLayout::eTransferDstOptimal, region);

    cmdList.end();
    submitAndWaitIdle(cmdList);

    transitionImageLayout(*this, image.image, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal,
                          vk::ImageLayout::eShaderReadOnlyOptimal);
}

void VulkanRHI::submitAndWaitIdle(CommandList &commandList) {
    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &*commandList.commandBuffer;
    m_graphicsQueue.submit({submitInfo}, nullptr);
    m_device.waitIdle();
}

Sampler VulkanRHI::createSampler() {
    vk::SamplerCreateInfo samplerInfo;
    samplerInfo.magFilter = vk::Filter::eLinear;
    samplerInfo.minFilter = vk::Filter::eLinear;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    samplerInfo.anisotropyEnable = true;
    samplerInfo.maxAnisotropy = m_physicalDevice.getProperties().limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    samplerInfo.unnormalizedCoordinates = false;
    samplerInfo.compareEnable = false;
    samplerInfo.compareOp = vk::CompareOp::eAlways;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.mipLodBias = 0.f;
    samplerInfo.minLod = 0.f;
    samplerInfo.maxLod = 0.f;

    vkr::Sampler sampler = m_device.createSampler(samplerInfo);
    return Sampler{std::move(sampler)};
}

void VulkanRHI::waitIdle() {
    m_device.waitIdle();
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

Shader::Shader(vkr::ShaderModule &&module) : module(std::move(module)) {

}

CommandList::CommandList(vkr::CommandBuffer &&commandBuffer, class VulkanRHI *rhi) : commandBuffer(
        std::move(commandBuffer)), rhi(rhi) {
}

void CommandList::begin() {
    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.pInheritanceInfo = nullptr;
    commandBuffer.begin(beginInfo);
}

void CommandList::end() {
    commandBuffer.end();
}

void CommandList::beginRenderPass(const RenderTarget &renderTarget) {

    vk::RenderPass renderPass = rhi->createRenderPass(renderTarget.imageIdx);
    vk::Framebuffer framebuffer = rhi->createFramebuffer(renderPass, renderTarget);

    vk::RenderPassBeginInfo renderPassBeginInfo;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = framebuffer;
    renderPassBeginInfo.renderArea.offset = vk::Offset2D(0, 0);
    renderPassBeginInfo.renderArea.extent = rhi->getExtent();

    std::array<vk::ClearValue, 2> clearValues;
    clearValues[0].color = vk::ClearColorValue(std::array<float, 4>{0.f, 0.f, 0.f, 0.f});
    clearValues[1].depthStencil = vk::ClearDepthStencilValue{1.0f, 0};
    renderPassBeginInfo.clearValueCount = clearValues.size();
    renderPassBeginInfo.pClearValues = clearValues.data();

    commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
}

void CommandList::draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance) {
    commandBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandList::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
                              uint32_t firstInstance) {
    commandBuffer.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}


void CommandList::BindPipeline(const RHIGraphicsPipelineRef &pipeline) {
    VulkanGraphicsPipeline *vkPipeline = static_cast<VulkanGraphicsPipeline *>(pipeline.get());
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, vkPipeline->PipelineHandle());
}

void CommandList::setViewport() {
    vk::Extent2D extent = rhi->getExtent();
    vk::Viewport viewport;
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.width = (float) extent.width;
    viewport.height = (float) extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    commandBuffer.setViewport(0, viewport);
}

void CommandList::setScissors() {
    vk::Extent2D extent = rhi->getExtent();

    vk::Rect2D scissor;
    scissor.offset = vk::Offset2D(0, 0);
    scissor.extent = extent;

    commandBuffer.setScissor(0, scissor);
}

void CommandList::endRenderPass() {
    commandBuffer.endRenderPass();
}

void CommandList::BindVertexBuffer(const RHIBufferRef &buf, int binding) {
    VulkanBuffer *buffer = static_cast<VulkanBuffer *>(buf.get());
    commandBuffer.bindVertexBuffers(binding, {buffer->BufferHandle()}, {0});
    ownedResources.emplace_back(buf);
}

void CommandList::BindIndexBuffer(const RHIBufferRef &buf) {
    VulkanBuffer *buffer = static_cast<VulkanBuffer *>(buf.get());
    commandBuffer.bindIndexBuffer(buffer->BufferHandle(), 0, vk::IndexType::eUint16);
    ownedResources.emplace_back(buf);
}

void CommandList::BindDescriptorSet(const RHIGraphicsPipelineRef &pipeline, const RHIDescriptorSetRef &descriptorSet, int binding) {
    VulkanGraphicsPipeline *vulkanPipeline = static_cast<VulkanGraphicsPipeline *>(pipeline.get());

    VulkanDescriptorSet *set = static_cast<VulkanDescriptorSet *>(descriptorSet.get());
commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, vulkanPipeline->LayoutHandle(), binding, vk::DescriptorSet(set->DescriptorSetHandle()),
                                     {});
ownedResources.emplace_back(descriptorSet);
}