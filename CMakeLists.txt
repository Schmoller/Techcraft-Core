cmake_minimum_required(VERSION 3.19)
project(tech_core CXX)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE ALL_SRC "src/*.cpp")

include_directories(./include ./src ./libs/stb ./libs/vk-mem-alloc/include)

add_library(tech ${ALL_SRC})