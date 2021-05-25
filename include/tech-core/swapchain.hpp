#pragma once

#include "forward.hpp"
#include <vulkan/vulkan.hpp>

namespace Engine {

class SwapChain {
public:
    SwapChain(vk::PhysicalDevice physicalDevice, VulkanDevice &device, vk::SurfaceKHR surface, vk::Extent2D size);
    ~SwapChain();

    void rebuild(vk::Extent2D windowExtent);
    void cleanup();

    size_t size() const { return images.size(); }

    vk::SwapchainKHR swapChain;
    vk::Extent2D extent;
    std::vector<vk::Image> images;
    std::vector<vk::ImageView> imageViews;
    vk::Format imageFormat;

private:
    // Provided
    vk::PhysicalDevice physicalDevice;
    vk::SurfaceKHR surface;
    VulkanDevice &device;

    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> presentModes;

    void setup(vk::Extent2D desiredExtent);
};

}