cmake_minimum_required(VERSION 3.19)
project(tech_core CXX)

set(CMAKE_CXX_STANDARD 20)
set(GLSL_COMPILER glslc)

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
include_directories(./libs/imgui)

set(IMGUI_SOURCES
        ./libs/imgui/imgui.cpp
        ./libs/imgui/imgui_demo.cpp
        ./libs/imgui/imgui_draw.cpp
        ./libs/imgui/imgui_tables.cpp
        ./libs/imgui/imgui_widgets.cpp
        ./libs/imgui/backends/imgui_impl_glfw.cpp
        )

# Target definition
add_library(tech ${ALL_SRC} ${IMGUI_SOURCES})
target_link_libraries(tech glfw ${Vulkan_LIBRARIES})

# Shaders
set(SHADER_SRC_DIR ${PROJECT_SOURCE_DIR}/shaders)
set(SHADER_BIN_DIR ${PROJECT_BINARY_DIR}/spirv)
SET(SHADER_LOCAL_INCLUDE ${PROJECT_SOURCE_DIR}/src/internal/packaged)
file(GLOB_RECURSE SHADERS_SRC "${SHADER_SRC_DIR}/*.glsl")

foreach (SHADER_FILE ${SHADERS_SRC})
    file(RELATIVE_PATH RELATIVE_PATH_TO_SHADER "${SHADER_SRC_DIR}" "${SHADER_FILE}")
    get_filename_component(FILE_NAME "${RELATIVE_PATH_TO_SHADER}" NAME_WLE)
    get_filename_component(RELATIVE_DIR_TO_SHADER "${RELATIVE_PATH_TO_SHADER}" DIRECTORY)

    string(MAKE_C_IDENTIFIER ${RELATIVE_PATH_TO_SHADER} SPV_HEADER_NAME)
    set(SPIRV "${SHADER_BIN_DIR}/${RELATIVE_DIR_TO_SHADER}/${FILE_NAME}.spv")
    set(SPV_HEADER ${SHADER_LOCAL_INCLUDE}/${SPV_HEADER_NAME}.h)

    add_custom_command(
            OUTPUT ${SPIRV} ${SPV_HEADER}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_BIN_DIR}/${RELATIVE_DIR_TO_SHADER}"
            COMMAND ${GLSL_COMPILER} -g ${SHADER_FILE} -o ${SPIRV}
            COMMAND ${CMAKE_COMMAND} -DINPUT_FILE=${SPIRV} -DOUTPUT=${SPV_HEADER} -DNAME=${SPV_HEADER_NAME} -P ${PROJECT_SOURCE_DIR}/cmake/pack_file.cmake
            DEPENDS ${SHADER_FILE}
    )

    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
    list(APPEND PACKAGED_INCLUDES ${SPV_HEADER})
endforeach (SHADER_FILE)

add_custom_target(
        tech_shaders
        DEPENDS ${SPIRV_BINARY_FILES}
)

add_dependencies(tech tech_shaders)
