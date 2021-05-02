cmake_minimum_required(VERSION 3.19)
project(tech_core CXX)

set(CMAKE_CXX_STANDARD 20)

# External Libraries
find_package(glfw3 3.3 REQUIRED STATIC)
find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)

include_directories(${glfw3_INCLUDE_DIRS})
include_directories(${Vulkan_INCLUDE_DIRS})

# Source files
file(GLOB_RECURSE ALL_SRC "src/*.cpp")
include_directories(./include ./src)

# Included libraries
include_directories(./libs/stb)
include_directories(./libs/vk-mem-alloc/include)

# Target definition
add_library(tech ${ALL_SRC})
target_link_libraries(tech glfw ${Vulkan_LIBRARIES})