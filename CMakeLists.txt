cmake_minimum_required(VERSION 3.22)

project(muffin)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(muffin)
add_subdirectory(thirdparty)

add_executable(main main.cpp stb_image.cpp)

target_link_libraries(main muffin VulkanRHI)