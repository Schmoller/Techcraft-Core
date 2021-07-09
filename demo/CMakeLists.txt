cmake_minimum_required(VERSION 3.19)
project(tech_core_demo)

set(CMAKE_CXX_STANDARD 20)
set(GLSL_COMPILER glslc)

include_directories(../include ../libs/vk-mem-alloc/include ../libs/stb ../libs/imgui)

add_executable(demo src/main.cpp src/demo.cpp)
target_link_libraries(demo tech)

set(SHADER_SRC_DIR ${PROJECT_SOURCE_DIR}/assets/shaders)
set(SHADER_BIN_DIR ${PROJECT_BINARY_DIR}/assets/shaders)

# Shaders
file(GLOB_RECURSE SHADERS_SRC "${SHADER_SRC_DIR}/*.glsl")

foreach (SHADER_FILE ${SHADERS_SRC})
    file(RELATIVE_PATH RELATIVE_PATH_TO_SHADER "${SHADER_SRC_DIR}" "${SHADER_FILE}")
    get_filename_component(FILE_NAME "${RELATIVE_PATH_TO_SHADER}" NAME_WLE)
    get_filename_component(RELATIVE_DIR_TO_SHADER "${RELATIVE_PATH_TO_SHADER}" DIRECTORY)

    set(SPIRV "${SHADER_BIN_DIR}/${RELATIVE_DIR_TO_SHADER}/${FILE_NAME}.spv")
    add_custom_command(
            OUTPUT ${SPIRV}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_BIN_DIR}/${RELATIVE_DIR_TO_SHADER}"
            COMMAND ${GLSL_COMPILER} ${SHADER_FILE} -o ${SPIRV}
            DEPENDS ${SHADER_FILE}
    )
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach (SHADER_FILE)

add_custom_target(
        demo_shaders
        DEPENDS ${SPIRV_BINARY_FILES}
)

add_dependencies(demo demo_shaders)

# Link resources
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/assets)
file(CREATE_LINK ${PROJECT_SOURCE_DIR}/assets/textures ${PROJECT_BINARY_DIR}/assets/textures SYMBOLIC)
file(CREATE_LINK ${PROJECT_SOURCE_DIR}/assets/models ${PROJECT_BINARY_DIR}/assets/models SYMBOLIC)