#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>
#include <optional>
#include <exception>

namespace Engine {

struct VulkanQueue {
    uint32_t index;
    vk::Queue queue;
};

class VulkanDevice {
    struct VulkanQueueIndices {
        std::optional<uint32_t> graphics;
        std::optional<uint32_t> present;
        std::optional<uint32_t> transfer;
        std::optional<uint32_t> compute;
    };

    public:
    /**
     * Attempts to initiate a vulkan device.
     * 
     * @throw DeviceNotSuitable if the device cannot be used
     */
    VulkanDevice(vk::PhysicalDevice device, vk::SurfaceKHR surface);
    ~VulkanDevice();
    VulkanDevice(const VulkanDevice &other) = delete;
    VulkanDevice(VulkanDevice &&other) = delete;

    void waitIdle() const;

    vk::Device device;
    VmaAllocator allocator;

    VulkanQueue graphicsQueue;
    VulkanQueue presentQueue;
    VulkanQueue transferQueue;
    VulkanQueue computeQueue;

    vk::CommandPool graphicsPool;
    vk::CommandPool computePool;
    vk::CommandPool transferPool;

    vk::Semaphore renderFinished;
    vk::Semaphore presentFinished;
    vk::Fence renderReady;

    private:
    // Provided
    vk::PhysicalDevice physicalDevice;
    vk::SurfaceKHR surface;

    VulkanQueueIndices findQueueIndices() const;
    bool hasAllRequiredExtensions() const;
};

class DeviceNotSuitable : public std::exception {};

}