add_subdirectory(rhi)

add_library(muffin Mesh.cpp Material.cpp RenderObject.cpp Renderer.cpp)
target_link_libraries(muffin VulkanRHI)