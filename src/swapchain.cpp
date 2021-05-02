#include "swapchain.hpp"

#include "vulkanutils.hpp"

namespace Engine {

SwapChain::SwapChain(vk::PhysicalDevice physicalDevice, VulkanDevice &device, vk::SurfaceKHR surface, vk::Extent2D size)
  : device(device), physicalDevice(physicalDevice), surface(surface)
{
    capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    formats = physicalDevice.getSurfaceFormatsKHR(surface);
    presentModes = physicalDevice.getSurfacePresentModesKHR(surface);

    setup(size);
}

SwapChain::~SwapChain() {

}

void SwapChain::rebuild(vk::Extent2D windowExtent) {
    cleanup();
    setup(windowExtent);
}

void SwapChain::setup(vk::Extent2D desiredExtent) {
    capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);

    vk::Extent2D actualExtent;
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        actualExtent = capabilities.currentExtent;
    } else {
        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, desiredExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, desiredExtent.height));
    }

    vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(formats);
    vk::PresentModeKHR presentMode = chooseSwapSurfacePresentMode(presentModes);

    uint32_t imageCount = capabilities.minImageCount + 1;

    if (capabilities.maxImageCount > 0) {
        imageCount = std::min(imageCount, capabilities.maxImageCount);
    }

    vk::SwapchainCreateInfoKHR createInfo(
        {},
        surface,
        imageCount,
        surfaceFormat.format,
        surfaceFormat.colorSpace,
        actualExtent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
        vk::SharingMode::eExclusive
    );

    if (device.graphicsQueue.index != device.presentQueue.index) {
        uint32_t queueFamilyIndices[] = {device.graphicsQueue.index, device.presentQueue.index};

        createInfo.setImageSharingMode(vk::SharingMode::eConcurrent);
        createInfo.setQueueFamilyIndexCount(2);
        createInfo.setPQueueFamilyIndices(queueFamilyIndices);
    }

    createInfo.setPreTransform(capabilities.currentTransform);
    createInfo.setPresentMode(presentMode);
    createInfo.setClipped(VK_TRUE);

    swapChain = device.device.createSwapchainKHR(createInfo);

    images = device.device.getSwapchainImagesKHR(swapChain);

    imageFormat = surfaceFormat.format;
    extent = actualExtent;

    // Now make image views for each swapchain image
    imageViews.clear();
    imageViews.reserve(imageCount);

    for (auto &image : images) {
        imageViews.push_back(
            device.device.createImageView({
                {},
                image,
                vk::ImageViewType::e2D,
                imageFormat,
                {},
                {
                    vk::ImageAspectFlagBits::eColor,
                    0,
                    1,
                    0,
                    1
                }
            })
        );
    }
}

void SwapChain::cleanup() {
    for (auto &imageView : imageViews) {
        device.device.destroyImageView(imageView);
    }

    device.device.destroySwapchainKHR(swapChain);
}

}