#include "tech-core/device.hpp"
#include "vulkanutils.hpp"

#include <vector>
#include <array>

#ifndef NDEBUG
    #define ENABLE_VALIDATION_LAYERS
#endif

namespace Engine {

// VulkanDevice
    VulkanDevice::VulkanDevice(vk::PhysicalDevice device, vk::Instance instance, vk::SurfaceKHR surface)
 : physicalDevice(device), surface(surface)
{
    // Ensure extensions are available
    if (!hasAllRequiredExtensions()) {
        throw DeviceNotSuitable();
    }

    // Ensure queues are available
    auto indices = findQueueIndices();
    if (!indices.graphics || !indices.present || !indices.compute || !indices.transfer) {
        throw DeviceNotSuitable();
    }

    graphicsQueue.index = *indices.graphics;
    presentQueue.index = *indices.present;
    computeQueue.index = *indices.compute;
    transferQueue.index = *indices.transfer;

    // Ensure the swap chain can be built
    auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    auto formats = physicalDevice.getSurfaceFormatsKHR(surface);
    auto presentModes = physicalDevice.getSurfacePresentModesKHR(surface);

    if (formats.empty() || presentModes.empty()) {
        throw DeviceNotSuitable();
    }

    // Device can now be initialised
    // Queues first
    std::vector<vk::DeviceQueueCreateInfo> queueCreation;
    float queuePriority = 1.0f;

    queueCreation.push_back(vk::DeviceQueueCreateInfo({}, graphicsQueue.index, 1, &queuePriority));
    if (presentQueue.index != graphicsQueue.index) {
        queueCreation.push_back(vk::DeviceQueueCreateInfo({}, presentQueue.index, 1, &queuePriority));
    }
    if (transferQueue.index != graphicsQueue.index && transferQueue.index != presentQueue.index) {
        queueCreation.push_back(vk::DeviceQueueCreateInfo({}, transferQueue.index, 1, &queuePriority));
    }
    if (computeQueue.index != graphicsQueue.index && computeQueue.index != presentQueue.index && computeQueue.index != transferQueue.index) {
        queueCreation.push_back(vk::DeviceQueueCreateInfo({}, computeQueue.index, 1, &queuePriority));
    }

    // Device creation
    vk::PhysicalDeviceFeatures deviceFeatures;
    deviceFeatures.setSamplerAnisotropy(VK_TRUE);

    std::array<const char*, 1> extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    vk::DeviceCreateInfo deviceCreateInfo(
        {},
        vkUseArray(queueCreation),
        0,
        nullptr,
        vkUseArray(extensions),
        &deviceFeatures
    );

    #ifdef ENABLE_VALIDATION_LAYERS
    const std::array<const char*, 1> validationLayers = {
        "VK_LAYER_LUNARG_standard_validation"
    };
    deviceCreateInfo.setEnabledLayerCount(static_cast<uint32_t>(validationLayers.size()));
    deviceCreateInfo.setPpEnabledLayerNames(validationLayers.data());
    #endif

    this->device = physicalDevice.createDevice(deviceCreateInfo);

    graphicsQueue.queue = this->device.getQueue(graphicsQueue.index, 0);
    presentQueue.queue = this->device.getQueue(presentQueue.index, 0);
    transferQueue.queue = this->device.getQueue(transferQueue.index, 0);
    computeQueue.queue = this->device.getQueue(computeQueue.index, 0);

    // Memory allocator
    VmaAllocatorCreateInfo allocInfo = {};
    allocInfo.physicalDevice = physicalDevice;
    allocInfo.device = this->device;
    allocInfo.instance = instance;
    
    if (vmaCreateAllocator(&allocInfo, &allocator) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create allocator");
    }

    // Command pools
    graphicsPool = this->device.createCommandPool({{vk::CommandPoolCreateFlagBits::eResetCommandBuffer}, graphicsQueue.index});
    computePool = this->device.createCommandPool({{vk::CommandPoolCreateFlagBits::eResetCommandBuffer}, computeQueue.index});

    if (graphicsQueue.index != transferQueue.index) {
        transferPool = this->device.createCommandPool({{vk::CommandPoolCreateFlagBits::eResetCommandBuffer}, transferQueue.index});
    } else {
        // Reuse the graphics pool when they are a shared queue
        transferPool = graphicsPool;
    }

    // Semaphores
    renderFinished = this->device.createSemaphore({});
    presentFinished = this->device.createSemaphore({});

    renderReady = this->device.createFence({vk::FenceCreateFlagBits::eSignaled});
}

VulkanDevice::~VulkanDevice() {
    this->device.destroyCommandPool(computePool);
    this->device.destroyCommandPool(graphicsPool);

    if (graphicsQueue.index != transferQueue.index) {
        this->device.destroyCommandPool(transferPool);
    }

    this->device.destroySemaphore(renderFinished);
    this->device.destroySemaphore(presentFinished);
    this->device.destroyFence(renderReady);

    if (allocator) {
        vmaDestroyAllocator(allocator);
    }

    if (device) {
        device.destroy();
    }
}

bool VulkanDevice::hasAllRequiredExtensions() const {
    auto extensions = physicalDevice.enumerateDeviceExtensionProperties();

    bool hasSurface = false;
    
    for (const auto &extension : extensions) {
        std::string_view name(extension.extensionName);

        if (name == VK_KHR_SWAPCHAIN_EXTENSION_NAME) {
            hasSurface = true;
        }
    }

    return hasSurface;
}

VulkanDevice::VulkanQueueIndices VulkanDevice::findQueueIndices() const {
    auto properties = physicalDevice.getQueueFamilyProperties();

    std::optional<uint32_t> graphicsIndex;
    std::optional<uint32_t> presentIndex;
    bool sharedPresent = false;
    std::optional<uint32_t> transferIndex;
    bool transferDedicated = false;
    std::optional<uint32_t> computeIndex;
    bool computeDedicated = false;

    vk::Bool32 canPresent;

    uint32_t index = 0;
    for (auto &family : properties) {
        if (family.queueCount > 0) {
            if (!graphicsIndex && family.queueFlags & vk::QueueFlagBits::eGraphics) {
                graphicsIndex = index;
            }

            // Transfer would like to be dedicated if possible
            if (family.queueFlags & vk::QueueFlagBits::eTransfer) {
                if (!transferDedicated) {
                    transferIndex = index;

                    if (family.queueFlags == static_cast<vk::QueueFlags>(vk::QueueFlagBits::eTransfer)) {
                        transferDedicated = true;
                    }
                }
            }

            // Compute would like to be dedicated if possible
            if (family.queueFlags & vk::QueueFlagBits::eCompute) {
                if (!computeDedicated) {
                    computeIndex = index;

                    if (family.queueFlags == static_cast<vk::QueueFlags>(vk::QueueFlagBits::eCompute)) {
                        computeDedicated = true;
                    }
                }
            }

            // Presentation. Try to put as part of the same queue as graphics
            if (!presentIndex || !sharedPresent) {
                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, index, surface, &canPresent);
                if (canPresent) {
                    if (graphicsIndex && *graphicsIndex == index) {
                        sharedPresent = true;
                    }
                    presentIndex = index;
                }
            }
        }

        ++index;
    }

    return {
        graphicsIndex,
        presentIndex,
        transferIndex,
        computeIndex
    };
}

void VulkanDevice::waitIdle() const {
    this->device.waitIdle();
}

}