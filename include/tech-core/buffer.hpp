#ifndef __BUFFER_HPP
#define __BUFFER_HPP

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

#include <deque>
#include <limits>
#include <list>
#include "forward.hpp"

// When defined, this will enable debug logging for buffers
// #define DEBUG_BUFFER

namespace vk {
enum class MemoryUsage {
    eUnknown = VMA_MEMORY_USAGE_UNKNOWN,
    eGPUOnly = VMA_MEMORY_USAGE_GPU_ONLY,
    eCPUOnly = VMA_MEMORY_USAGE_CPU_ONLY,
    eCPUToGPU = VMA_MEMORY_USAGE_CPU_TO_GPU,
    eGPUToCPU = VMA_MEMORY_USAGE_GPU_TO_CPU,
};
}

namespace Engine {

struct BufferFence {
    std::unique_ptr<Buffer> buffer;
    vk::Fence fence;
    vk::UniqueFence uniqueFence;
};

class BufferManager {
    friend class RenderEngine;
public:
    explicit BufferManager(VulkanDevice &device);

    /**
     * General purpose buffer aquisition.
     * Do not use for staging buffers, use the aquireStaging() method.
     */
    std::unique_ptr<Buffer> aquire(
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        vk::MemoryUsage memoryUsage
    );

    /**
     * General purpose buffer aquisition.
     * Do not use for staging buffers, use the aquireStaging() method.
     */
    std::shared_ptr<Buffer> aquireShared(unsigned long size, vk::BufferUsageFlags usage, vk::MemoryUsage memoryUsage);

    /**
     * Divisible buffer aquisition.
     * This buffer can have segments allocated for specific tasks
     */
    std::unique_ptr<DivisibleBuffer> aquireDivisible(
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        vk::MemoryUsage memoryUsage
    );

    /**
     * Divisible buffer aquisition.
     * This buffer can have segments allocated for specific tasks
     */
    std::shared_ptr<DivisibleBuffer> aquireDivisibleShared(
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        vk::MemoryUsage memoryUsage
    );

    /**
     * Releases a buffer after the next frame has completed.
     * This should be used for releasing any buffer that is used in the
     * render pipeline
     */
    void releaseAfterFrame(std::unique_ptr<Buffer> buffer);

    /**
     * Aquire a staging buffer of the required size.
     * A staging buffer has the following characteristics:
     * - It is CPU local
     * - Can be used as a transfer source
     */
    std::unique_ptr<Buffer> aquireStaging(vk::DeviceSize size);
    /**
     * Releases a buffer
     * If a fence is provided, the buffer will only be released when that fence is complete.
     */
    void release(std::unique_ptr<Buffer> &buffer, vk::Fence onlyAfter = vk::Fence());
    /**
     * Releases a buffer when that fence is complete.
     * The ownership of the fence is taken by this manager and will be freed after the buffer.
     */
    void release(std::unique_ptr<Buffer> &buffer, vk::UniqueFence onlyAfter);

private:
    void processActions();

    // Provided fields
    VulkanDevice &device;

    // State
    std::deque<std::unique_ptr<Buffer>> nextFrameRelease;
    std::deque<BufferFence> fenceRelease;

    /*
    TODO: This is where we left off.
    The idea here is: 
    Instead of creating a staging buffer as a stack variable and then waiting until the transfer is complete,
    we use this class to aquire a buffer of the correct size for staging use.
    We write whatever to it and queue it up to be transferred.
    After queueing it up, submit the task and release the staging buffer back to this class
    along with a fence. We will then use the fence to determine if the resource can be returned or not.
    We dont actually need to wait, we can just query the status using device.waitForFences with a 0 timeout.
    https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/vkWaitForFences.html

    The check for buffers to be released can be done after each frame I think.
    */
};

class Buffer {
public:
    Buffer();
    Buffer(VmaAllocator allocator, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryUsage targetUsage);
    virtual ~Buffer();

    vk::DeviceSize getSize() const { return size; }

    virtual void
    allocate(VmaAllocator allocator, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryUsage targetUsage);
    void destroy();

    void copyIn(const void *data, vk::DeviceSize offset, vk::DeviceSize size);

    inline void copyIn(const void *data, vk::DeviceSize size = VK_WHOLE_SIZE) {
        if (size == VK_WHOLE_SIZE) {
            size = this->size;
        }

        copyIn(data, 0, size);
    }

    void copyOut(void *dest, vk::DeviceSize offset, vk::DeviceSize size);

    inline void copyOut(void *dest, vk::DeviceSize size = VK_WHOLE_SIZE) {
        if (size == VK_WHOLE_SIZE) {
            size = this->size;
        }

        copyOut(dest, 0, size);
    }

    void transfer(
        VkCommandBuffer commandBuffer, Buffer &target, vk::DeviceSize srcOffset, vk::DeviceSize destOffset,
        vk::DeviceSize size
    );

    inline void transfer(VkCommandBuffer commandBuffer, Buffer &target, vk::DeviceSize offset, vk::DeviceSize size) {
        transfer(commandBuffer, target, offset, 0, size);
    }

    inline void transfer(VkCommandBuffer commandBuffer, Buffer &target, vk::DeviceSize size = VK_WHOLE_SIZE) {
        if (size == VK_WHOLE_SIZE) {
            size = this->size;
        }
        transfer(commandBuffer, target, 0, 0, size);
    }

    inline void map(void **data) {
        vmaMapMemory(allocator, allocation, data);
    }

    inline void unmap() {
        vmaUnmapMemory(allocator, allocation);
    }

    /**
     * Flushes the entire buffer.
     * Only applicable for host visible but non-coherent buffers
     */
    void flush();
    /**
     * Flushes a range of memory.
     * Only applicable for host visible but non-coherent buffers
     */
    void flushRange(vk::DeviceSize start, vk::DeviceSize size);

    const vk::Buffer buffer() const {
        return internalBuffer;
    }

    const vk::Buffer *bufferArray() const {
        return &internalBuffer;
    }

protected:
    VkDeviceSize size;
    vk::Buffer internalBuffer;

private:
    VmaAllocator allocator;
    VmaAllocation allocation;
};

#define ALLOCATION_FAILED std::numeric_limits<vk::DeviceSize>::max()
struct FreeSpace {
    vk::DeviceSize offset;
    vk::DeviceSize size;
};

class DivisibleBuffer : public Buffer {
public:
    DivisibleBuffer();
    DivisibleBuffer(
        VmaAllocator allocator, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryUsage targetUsage
    );
    virtual ~DivisibleBuffer();

    virtual void
    allocate(VmaAllocator allocator, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryUsage targetUsage);

    /**
     * Releases a previously held region of the buffer
     */
    void freeSection(vk::DeviceSize offset, vk::DeviceSize size);
    template<typename T>
    void freeSection(vk::DeviceSize offset);

    /**
     * Allocates a new region of the buffer.
     * @returns The offset in the buffer. Returns ALLOCATION_FAILED if allocation failed
     */
    vk::DeviceSize allocateSection(vk::DeviceSize size);
    template<typename T>
    vk::DeviceSize allocateSection();

private:
    std::list<FreeSpace> freeSpaceTracking;
};

}
#endif