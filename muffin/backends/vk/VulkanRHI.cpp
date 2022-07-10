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


vkr::PipelineLayout createPipelineLayout(const vkr::Device &device) {
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pSetLayouts = nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    vkr::PipelineLayout pipelineLayout(device, pipelineLayoutInfo);
    return pipelineLayout;
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

    vk::SubpassDescription subpass;
    subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    vk::SubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    //dependency.srcAccessMask = 0;

    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

    vk::RenderPassCreateInfo renderPassInfo;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
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

    vk::PipelineVertexInputStateCreateInfo vertexInput;
    vertexInput.vertexBindingDescriptionCount = 0;
    vertexInput.pVertexBindingDescriptions = nullptr;
    vertexInput.vertexAttributeDescriptionCount = 0;
    vertexInput.pVertexBindingDescriptions = nullptr;

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
    rasterizer.frontFace = vk::FrontFace::eClockwise;
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

    vkr::PipelineLayout layout = createPipelineLayout(m_device);

    vk::RenderPass renderPass = createRenderPass();

    vk::GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = *layout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = nullptr;
    pipelineInfo.basePipelineIndex = -1;

    vkr::Pipeline pipeline(m_device, nullptr, pipelineInfo);

    return GraphicsPipeline{std::move(pipeline), std::move(layout)};
}

vk::Framebuffer
VulkanRHI::createFramebuffer(const vk::RenderPass &renderPass, const RenderTarget &renderTarget) {
    vk::ImageView attachments[] = {
            renderTarget.swapchainImg
    };
    vk::FramebufferCreateInfo framebufferInfo;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
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
        m_imageAvailableSemaphore(nullptr),
        m_renderFinishedSemaphore(nullptr),
        m_inFlightFence(nullptr) {

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

    vk::SemaphoreCreateInfo semaphoreInfo;
    m_imageAvailableSemaphore = vkr::Semaphore(m_device, semaphoreInfo);
    m_renderFinishedSemaphore = vkr::Semaphore(m_device, semaphoreInfo);

    vk::FenceCreateInfo fenceInfo;
    fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
    m_inFlightFence = vkr::Fence(m_device, fenceInfo);
}

#include <iostream>

Shader VulkanRHI::createShader(const std::vector<uint32_t> &code) {
    vk::ShaderModuleCreateInfo createInfo;
    createInfo.codeSize = code.size() * sizeof(uint32_t);
    createInfo.pCode = code.data();
    vkr::ShaderModule shaderMoule(m_device, createInfo);

    spirv_cross::Compiler comp(code);

    auto resources = comp.get_shader_resources();

    std::cout << "builtin_inputs " << resources.builtin_inputs.size() << std::endl;
    for (auto &r: resources.builtin_inputs) {
        std::cout << r.resource.name << std::endl;
    }

    std::cout << "builtin_outputs " << resources.builtin_outputs.size() << std::endl;
    for (auto &r: resources.builtin_outputs) {
        std::cout << r.resource.name << std::endl;
    }

    std::cout << "stage_inputs " << resources.stage_inputs.size() << std::endl;
    for (auto &r: resources.stage_inputs) {
        std::cout << r.name << std::endl;
    }

    std::cout << "stage_outputs " << resources.stage_outputs.size() << std::endl;
    for (auto &r: resources.stage_outputs) {
        std::cout << r.name << std::endl;
    }

    return Shader{std::move(shaderMoule)};
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

    vk::ClearValue clearValue(vk::ClearColorValue(std::array<float, 4>{0.f, 0.f, 0.f, 0.f}));
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearValue;

    commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
}

void CommandList::draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance) {
    commandBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
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

GraphicsPipeline::GraphicsPipeline(vkr::Pipeline &&pipeline, vkr::PipelineLayout &&layout)
        : pipeline(
        std::move(pipeline)), layout(std::move(layout)) {

}
