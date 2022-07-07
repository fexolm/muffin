cmake_minimum_required(VERSION 3.22)

project(muffin)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(muffin)

add_executable(main main.cpp)

target_link_libraries(main VulkanRHI)