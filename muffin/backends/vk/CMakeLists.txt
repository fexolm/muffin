find_package(Vulkan REQUIRED)
find_package(SDL2 CONFIG REQUIRED)

add_library(VulkanRHI VulkanRHI.cpp)
target_include_directories(VulkanRHI PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(VulkanRHI ${VULKAN_LIBRARIES} SDL2::SDL2 spirv-cross-cpp)