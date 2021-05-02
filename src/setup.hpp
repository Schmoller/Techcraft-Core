#ifndef SETUP_HPP
#define SETUP_HPP

#ifdef SETUP_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif

#include <vulkan/vulkan.hpp>

#include "vk_mem_alloc.h"
#include "stb_image.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#endif