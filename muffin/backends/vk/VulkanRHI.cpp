#include "VulkanRHI.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <spirv_cross/spirv_cross.hpp>

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

Texture
createImage(const vkr::Device &device, const vkr::PhysicalDevice &physicalDevice, uint32_t width, uint32_t height,
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

    return Texture{std::move(image), std::move(memory), std::move(view)};
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
createPipelineLayout(const vkr::Device &device, const vkr::DescriptorSetLayout &descriptorSetLayout) {
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &*descriptorSetLayout;
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

Texture
createDepthImage(VulkanRHI &rhi, const vkr::Device &device, const vkr::PhysicalDevice &physicalDevice, uint32_t width,
                 uint32_t height) {
    vk::Format depthFormat = findDepthFormat(physicalDevice);
    Texture texture = createImage(device, physicalDevice, width, height, depthFormat, vk::ImageTiling::eOptimal,
                                  vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                  vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eDepth);

    transitionImageLayout(rhi, texture.image, depthFormat, vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eDepthStencilAttachmentOptimal);
    return texture;
}

vk::RenderPass VulkanRHI::createRenderPass() {
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
    //dependency.srcAccessMask = 0;

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

    m_renderPassCache.emplace_back(m_device, renderPassInfo);
    return *m_renderPassCache.back();
}

GraphicsPipeline VulkanRHI::createGraphicsPipeline(const GraphicsPipelineCreateInfo &info) {
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
    vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
    vertShaderStageInfo.module = *info.vertexShader->module;
    vertShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
    fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
    fragShaderStageInfo.module = *info.fragmentShader->module;
    fragShaderStageInfo.pName = "main";

    vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    vk::VertexInputBindingDescription bindingDescription;
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = vk::VertexInputRate::eVertex;

    vk::VertexInputAttributeDescription posDescription;
    posDescription.binding = 0;
    posDescription.location = 0;
    posDescription.format = vk::Format::eR32G32B32Sfloat;
    posDescription.offset = offsetof(Vertex, pos);

    vk::VertexInputAttributeDescription colorDescription;
    colorDescription.binding = 0;
    colorDescription.location = 1;
    colorDescription.format = vk::Format::eR32G32B32Sfloat;
    colorDescription.offset = offsetof(Vertex, color);

    vk::VertexInputAttributeDescription texCoordDescription;
    texCoordDescription.binding = 0;
    texCoordDescription.location = 2;
    texCoordDescription.format = vk::Format::eR32G32Sfloat;
    texCoordDescription.offset = offsetof(Vertex, texCoord);

    vk::VertexInputAttributeDescription attributeDescriptions[] = {
            posDescription, colorDescription, texCoordDescription
    };

    vk::PipelineVertexInputStateCreateInfo vertexInput;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &bindingDescription;
    vertexInput.vertexAttributeDescriptionCount = 3;
    vertexInput.pVertexAttributeDescriptions = attributeDescriptions;

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = false;

    vk::Viewport viewport;
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.width = m_extent.width;
    viewport.height = m_extent.height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;

    vk::Rect2D scissor;
    scissor.offset = vk::Offset2D(0, 0);
    scissor.extent = m_extent;

    std::vector<vk::DynamicState> dynamicStates = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor,
    };
    vk::PipelineDynamicStateCreateInfo dynamicState;
    dynamicState.dynamicStateCount = (uint32_t) dynamicStates.size();
    dynamicState.pDynamicStates = dynamicStates.data();

    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.pScissors = &scissor;

    vk::PipelineRasterizationStateCreateInfo rasterizer;
    rasterizer.depthClampEnable = false;
    rasterizer.rasterizerDiscardEnable = false;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
    rasterizer.depthBiasEnable = false;
    rasterizer.depthBiasConstantFactor = 0.f;
    rasterizer.depthBiasClamp = 0.f;
    rasterizer.depthBiasSlopeFactor = 0.f;

    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.sampleShadingEnable = false;
    multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
    multisampling.minSampleShading = 1.f;
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = false;
    multisampling.alphaToOneEnable = false;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.colorWriteMask =
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA;
    colorBlendAttachment.blendEnable = false;
    colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne;
    colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero;
    colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
    colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
    colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
    colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

    vk::PipelineColorBlendStateCreateInfo colorBlending;
    colorBlending.logicOpEnable = false;
    colorBlending.logicOp = vk::LogicOp::eCopy;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    vk::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.depthTestEnable = true;
    depthStencil.depthWriteEnable = true;
    depthStencil.depthCompareOp = vk::CompareOp::eLess;
    depthStencil.depthBoundsTestEnable = false;
    depthStencil.minDepthBounds = 0.f;
    depthStencil.maxDepthBounds = 1.f;
    depthStencil.stencilTestEnable = false;
    depthStencil.front = vk::StencilOpState{};
    depthStencil.back = vk::StencilOpState{};

    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    bindings.insert(bindings.end(), info.vertexShader->bindings.begin(), info.vertexShader->bindings.end());
    bindings.insert(bindings.end(), info.fragmentShader->bindings.begin(), info.fragmentShader->bindings.end());

    vkr::DescriptorSetLayout descriptorSetLayout = createDescriptorSetLayout(m_device, bindings);

    vkr::PipelineLayout layout = createPipelineLayout(m_device, descriptorSetLayout);

    vk::RenderPass renderPass = createRenderPass();

    vk::GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = *layout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = nullptr;
    pipelineInfo.basePipelineIndex = -1;

    vkr::Pipeline pipeline(m_device, nullptr, pipelineInfo);

    return GraphicsPipeline{std::move(pipeline), std::move(layout), std::move(descriptorSetLayout)};
}

vk::Framebuffer
VulkanRHI::createFramebuffer(const vk::RenderPass &renderPass, const RenderTarget &renderTarget) {
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

    m_frameBuffersCache.emplace_back(m_device, framebufferInfo);
    return *m_frameBuffersCache.back();
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
        m_imageAvailableSemaphore(nullptr),
        m_renderFinishedSemaphore(nullptr),
        m_inFlightFence(nullptr),
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
    m_imageAvailableSemaphore = vkr::Semaphore(m_device, semaphoreInfo);
    m_renderFinishedSemaphore = vkr::Semaphore(m_device, semaphoreInfo);

    vk::FenceCreateInfo fenceInfo;
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
    m_inFlightFence = vkr::Fence(m_device, fenceInfo);

    m_depthImage = createDepthImage(*this, m_device, m_physicalDevice, m_extent.width, m_extent.height);
}

#include <iostream>

Shader VulkanRHI::createShader(const std::vector<uint32_t> &code, ShaderType type) {
    vk::ShaderModuleCreateInfo createInfo;
    createInfo.codeSize = code.size() * sizeof(uint32_t);
    createInfo.pCode = code.data();
    vkr::ShaderModule shaderMoule(m_device, createInfo);

    spirv_cross::Compiler comp(code);

    auto resources = comp.get_shader_resources();

    auto res = Shader{std::move(shaderMoule)};

    for (auto &ub: resources.uniform_buffers) {
        //auto set = comp.get_decoration(ub.id, spv::DecorationDescriptorSet);
        auto binding = comp.get_decoration(ub.id, spv::DecorationBinding);
        vk::DescriptorSetLayoutBinding layoutBinding;
        layoutBinding.binding = binding;
        layoutBinding.descriptorCount = 1;
        layoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
        switch (type) {
            case ShaderType::Vertex:
                layoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
                break;
            case ShaderType::Fragment:
                layoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
                break;
        }
        layoutBinding.pImmutableSamplers = nullptr;

        res.bindings.push_back(layoutBinding);
    }

    for (auto &ub: resources.sampled_images) {
        auto binding = comp.get_decoration(ub.id, spv::DecorationBinding);
        vk::DescriptorSetLayoutBinding layoutBinding;
        layoutBinding.binding = binding;
        layoutBinding.descriptorCount = 1;
        layoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;

        switch (type) {
            case ShaderType::Vertex:
                layoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
                break;
            case ShaderType::Fragment:
                layoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
                break;
        }
        layoutBinding.pImmutableSamplers = nullptr;
        res.bindings.push_back(layoutBinding);
    }

    return res;
}

void VulkanRHI::submit(CommandList &commandList) {
    vk::SubmitInfo submitInfo;
    vk::Semaphore waitSemaphores[] = {*m_imageAvailableSemaphore};
    vk::CommandBuffer commandBuffers = {*commandList.commandBuffer};
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    vk::Semaphore signalSemaphores[] = {*m_renderFinishedSemaphore};
    submitInfo.setWaitSemaphores(waitSemaphores)
            .setWaitDstStageMask(waitStages)
            .setCommandBuffers(commandBuffers)
            .setSignalSemaphores(signalSemaphores);

    m_graphicsQueue.submit({submitInfo}, *m_inFlightFence);
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
    m_device.resetFences({*m_inFlightFence});
    m_currentSwapchainImgIdx = m_swapchain.acquireNextImage(UINT64_MAX, *m_imageAvailableSemaphore, nullptr).second;

    return RenderTarget{*m_swapchainImageViews[m_currentSwapchainImgIdx]};
}

void VulkanRHI::endFrame() {
    vk::Semaphore signalSemaphores[] = {*m_renderFinishedSemaphore};
    vk::SwapchainKHR swapChains[] = {*m_swapchain};
    vk::PresentInfoKHR presentInfo;
    presentInfo.pSwapchains = swapChains;
    presentInfo.swapchainCount = 1;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.pResults = nullptr;

    presentInfo.pImageIndices = &m_currentSwapchainImgIdx;

    m_graphicsQueue.presentKHR(presentInfo);
    m_device.waitForFences({*m_inFlightFence}, true, UINT64_MAX);
}

Buffer VulkanRHI::createBuffer(size_t size, const BufferInfo &info) {
    vk::BufferCreateInfo bufferInfo;
    bufferInfo.size = size;

    switch (info.usage) {
        case BufferUsage::Index:
            bufferInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer;
            break;
        case BufferUsage::Vertex:
            bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
            break;
        case BufferUsage::Uniform:
            bufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
            break;
        case BufferUsage::Staging:
            bufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
            break;
    }

    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    vkr::Buffer buffer(m_device, bufferInfo);

    auto memReq = buffer.getMemoryRequirements();

    vk::MemoryAllocateInfo allocInfo;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = findMemoryType(m_physicalDevice, memReq.memoryTypeBits,
                                               vk::MemoryPropertyFlagBits::eHostVisible |
                                               vk::MemoryPropertyFlagBits::eHostCoherent);

    auto memory = m_device.allocateMemory(allocInfo);
    buffer.bindMemory(*memory, 0);
    return Buffer{std::move(buffer), std::move(memory)};
}

DescriptorSet VulkanRHI::createDescriptorSet(const GraphicsPipeline &pipeline) {
    vk::DescriptorSetAllocateInfo allocInfo;
    allocInfo.descriptorPool = *m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &*pipeline.descriptorSetLayout;
    std::vector<vkr::DescriptorSet> descriptorSets = m_device.allocateDescriptorSets(allocInfo);

    return DescriptorSet{std::move(descriptorSets[0])};
}

void VulkanRHI::updateDescriptorSet(DescriptorSet &set, Buffer &buffer, Texture &texture, Sampler &sampler, int size) {
    vk::DescriptorBufferInfo bufferInfo;
    bufferInfo.buffer = *buffer.buffer;
    bufferInfo.offset = 0;
    bufferInfo.range = size;

    vk::DescriptorImageInfo imageInfo;
    imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    imageInfo.imageView = *texture.view;
    imageInfo.sampler = *sampler.sampler;

    vk::WriteDescriptorSet writes[2];
    writes[0].dstSet = *set.descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].dstArrayElement = 0;
    writes[0].descriptorType = vk::DescriptorType::eUniformBuffer;
    writes[0].descriptorCount = 1;
    writes[0].pBufferInfo = &bufferInfo;
    writes[0].pImageInfo = nullptr;
    writes[0].pTexelBufferView = nullptr;

    writes[1].dstSet = *set.descriptorSet;
    writes[1].dstBinding = 1;
    writes[1].dstArrayElement = 0;
    writes[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
    writes[1].descriptorCount = 1;
    writes[1].pImageInfo = &imageInfo;

    m_device.updateDescriptorSets(writes, {});
}

Texture VulkanRHI::createTexture(uint32_t width, uint32_t height) {
    return createImage(m_device, m_physicalDevice, width, height, vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal,
                       vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                       vk::MemoryPropertyFlagBits::eDeviceLocal, vk::ImageAspectFlagBits::eColor);
}

void VulkanRHI::copyBuffer(const Buffer &srcBuffer, Buffer &dstBuffer, int size) {
    auto cmdList = createCommandList();
    cmdList.begin();

    vk::BufferCopy copyRegion;
    copyRegion.size = size;

    cmdList.commandBuffer.copyBuffer(*srcBuffer.buffer, *dstBuffer.buffer, copyRegion);

    cmdList.end();
    submitAndWaitIdle(cmdList);
}

void VulkanRHI::copyBufferToTexture(const Buffer &buf, Texture &texture, uint32_t width, uint32_t height) {
    transitionImageLayout(*this, texture.image, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eUndefined,
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

    cmdList.commandBuffer.copyBufferToImage(*buf.buffer, *texture.image, vk::ImageLayout::eTransferDstOptimal, region);

    cmdList.end();
    submitAndWaitIdle(cmdList);

    transitionImageLayout(*this, texture.image, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eTransferDstOptimal,
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

    vk::RenderPass renderPass = rhi->createRenderPass();
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


void CommandList::bindPipeline(const GraphicsPipeline &pipeline) {
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline.pipeline);
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

void CommandList::bindVertexBuffer(const Buffer &buf) {
    commandBuffer.bindVertexBuffers(0, {*buf.buffer}, {0});
}

void CommandList::bindIndexBuffer(const Buffer &buf) {
    commandBuffer.bindIndexBuffer(*buf.buffer, 0, vk::IndexType::eUint16);
}

void CommandList::bindDescriptorSet(const GraphicsPipeline &pipeline, const DescriptorSet &set) {
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline.layout, 0, *set.descriptorSet,
                                     {});
}

GraphicsPipeline::GraphicsPipeline(vkr::Pipeline &&pipeline, vkr::PipelineLayout &&layout,
                                   vkr::DescriptorSetLayout &&descriptorSetLayout)
        : pipeline(
        std::move(pipeline)), layout(std::move(layout)), descriptorSetLayout(std::move(descriptorSetLayout)) {

}

void Buffer::fill(void *data, size_t size) {
    void *devicePtr = memory.mapMemory(0, size);
    memcpy(devicePtr, data, size);
    memory.unmapMemory();
}
