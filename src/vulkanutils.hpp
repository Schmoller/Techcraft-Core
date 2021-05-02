#ifndef VULKANUTILS_HPP
#define VULKANUTILS_HPP

#include <vulkan/vulkan.hpp>
#include <vector>

namespace Engine {

/**
 * Allows an array or vector to be used in place of a size and data pointer.
 */
#define vkUseArray(array) static_cast<uint32_t>(array.size()), array.data()

const std::vector<const char*> VALIDATION_LAYERS = {
    "VK_LAYER_LUNARG_standard_validation"
};

bool checkValidationLayerSupport();

vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &formats);
vk::PresentModeKHR chooseSwapSurfacePresentMode(const std::vector<vk::PresentModeKHR> &modes);

bool hasStencilComponent(vk::Format format);

vk::ShaderModule createShaderModule(vk::Device device, const std::vector<char> &code);
std::vector<char> readFile(const std::string &filename);

void* alignedAlloc(size_t size, size_t alignment);
void alignedFree(void* data);

// Candidates:
/*
SwapChainSupportDetails RenderEngine::querySwapChainSupport(VkPhysicalDevice device) {
*/

}

#endif