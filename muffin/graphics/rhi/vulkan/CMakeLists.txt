find_package(Vulkan REQUIRED)
find_package(SDL2 CONFIG REQUIRED)

add_library(
    VulkanRHI 

    VulkanRHI.cpp 
    VulkanBuffer.cpp 
    VulkanDescriptorSet.cpp 
    VulkanGraphicsPipeline.cpp 
    VulkanDescriptorPool.cpp 
    VulkanDevice.cpp 
    VulkanRenderPass.cpp 
    VulkanInstance.cpp
    VulkanWindow.cpp
    VulkanImage.cpp
    VulkanSampler.cpp
    VulkanShader.cpp
    VulkanCommandPool.cpp
    VulkanCommandList.cpp
    VulkanRenderTarget.cpp
    RHI.cpp
    )
target_include_directories(VulkanRHI PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(VulkanRHI Vulkan::Vulkan SDL2::SDL2 spirv-cross-cpp imgui editor)