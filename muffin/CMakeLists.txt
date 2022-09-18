add_subdirectory(rhi)
add_subdirectory(editor)

add_library(muffin Mesh.cpp Material.cpp RenderObject.cpp Renderer.cpp Scene.cpp)
target_link_libraries(muffin VulkanRHI)