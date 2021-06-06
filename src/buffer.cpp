#include "tech-core/buffer.hpp"
#include "tech-core/device.hpp"
#include <iostream>

namespace Engine {

BufferManager::BufferManager(VulkanDevice &device)
    : device(device) {}

std::unique_ptr<Buffer> BufferManager::aquire(
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    vk::MemoryUsage memoryUsage
) {
    return std::make_unique<Buffer>(device.allocator, size, usage, memoryUsage);
}

std::shared_ptr<Buffer> BufferManager::aquireShared(
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    vk::MemoryUsage memoryUsage
) {
    return std::make_shared<Buffer>(device.allocator, size, usage, memoryUsage);
}

std::unique_ptr<DivisibleBuffer> BufferManager::aquireDivisible(
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    vk::MemoryUsage memoryUsage
) {
    return std::make_unique<DivisibleBuffer>(device.allocator, size, usage, memoryUsage);
}

std::shared_ptr<DivisibleBuffer> BufferManager::aquireDivisibleShared(
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    vk::MemoryUsage memoryUsage
) {
    return std::make_shared<DivisibleBuffer>(device.allocator, size, usage, memoryUsage);
}

void BufferManager::releaseAfterFrame(std::unique_ptr<Buffer> buffer) {
#ifdef DEBUG_BUFFER
    std::cout << "release buffer after frame " << buffer->buffer() << std::endl;
#endif
    nextFrameRelease.push_back(std::move(buffer));
}

std::unique_ptr<Buffer> BufferManager::aquireStaging(vk::DeviceSize size) {
    return std::make_unique<Buffer>(
        device.allocator,
        size,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryUsage::eCPUOnly
    );
}

void BufferManager::release(std::unique_ptr<Buffer> &buffer, vk::Fence onlyAfter) {
    if (onlyAfter) {
#ifdef DEBUG_BUFFER
        std::cout << "release buffer after fence " << buffer->buffer() << " " << onlyAfter << std::endl;
#endif
        fenceRelease.push_back(
            {
                std::move(buffer),
                onlyAfter,
                vk::UniqueFence()
            }
        );
    } else {
        buffer.reset();
    }
}

void BufferManager::release(std::unique_ptr<Buffer> &buffer, vk::UniqueFence onlyAfter) {
    if (onlyAfter) {
#ifdef DEBUG_BUFFER
        std::cout << "release buffer after fence(own) " << buffer->buffer() << " " << onlyAfter.get() << std::endl;
#endif
        fenceRelease.push_back(
            {
                std::move(buffer),
                vk::Fence(),
                std::move(onlyAfter)
            }
        );
    } else {
        buffer.reset();
    }
}

void BufferManager::processActions() {
    // Release the ones for the next frame
    nextFrameRelease.clear();

    // Release after fences
    fenceRelease.erase(
        std::remove_if(
            fenceRelease.begin(),
            fenceRelease.end(),
            [this](BufferFence &pair) {
                // TODO: Is this predicate opposite?
                if (pair.uniqueFence) {
                    return this->device.device.waitForFences(1, &pair.uniqueFence.get(), VK_FALSE, 0) ==
                        vk::Result::eSuccess;
                } else {
                    return this->device.device.waitForFences(1, &pair.fence, VK_FALSE, 0) == vk::Result::eSuccess;
                }
            }
        ),
        fenceRelease.end()
    );
}


Buffer::Buffer()
    : allocator(VK_NULL_HANDLE), size(0), allocation(VK_NULL_HANDLE) {}

Buffer::Buffer(VmaAllocator allocator, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryUsage targetUsage)
    : Buffer() {

    allocate(allocator, size, usage, targetUsage);
}

Buffer::~Buffer() {
    destroy();
}

void
Buffer::allocate(VmaAllocator allocator, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryUsage targetUsage) {
    this->allocator = allocator;
    this->size = size;

    VkBufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = size;
    createInfo.usage = static_cast<VkBufferUsageFlags>(usage);
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = static_cast<VmaMemoryUsage>(targetUsage);

    VkBuffer temp;
    if (vmaCreateBuffer(allocator, &createInfo, &allocInfo, &temp, &allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate buffer");
    }

    internalBuffer = vk::Buffer(temp);

#ifdef DEBUG_BUFFER
    std::cout << "Created Buffer " << internalBuffer << std::endl;
#endif
}

void Buffer::destroy() {
    if (allocator && allocation) {
#ifdef DEBUG_BUFFER
        // std::cout << "Destroyed buffer " << internalBuffer << std::endl;
#endif
        vmaDestroyBuffer(allocator, internalBuffer, allocation);

        allocator = VK_NULL_HANDLE;
        allocation = VK_NULL_HANDLE;
    }
}

void Buffer::copyIn(const void *data, vk::DeviceSize offset, vk::DeviceSize size) {
    void *bufferData;
    vmaMapMemory(allocator, allocation, &bufferData);
    memcpy(bufferData + offset, data, size);
    vmaUnmapMemory(allocator, allocation);
}

void Buffer::copyOut(void *dest, vk::DeviceSize offset, vk::DeviceSize size) {
    void *bufferData;
    vmaMapMemory(allocator, allocation, &bufferData);
    memcpy(dest, bufferData + offset, size);
    vmaUnmapMemory(allocator, allocation);
}

void Buffer::transfer(
    VkCommandBuffer commandBuffer, Buffer &target, vk::DeviceSize srcOffset, vk::DeviceSize destOffset,
    vk::DeviceSize size
) {
    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = srcOffset;
    copyRegion.dstOffset = destOffset;
    copyRegion.size = size;

    vkCmdCopyBuffer(commandBuffer, internalBuffer, target.internalBuffer, 1, &copyRegion);
}

/**
 * Flushes the entire buffer.
 * Only applicable for host visible but non-coherent buffers
 */
void Buffer::flush() {
    vmaFlushAllocation(allocator, allocation, 0, size);
}

/**
 * Flushes a range of memory.
 * Only applicable for host visible but non-coherent buffers
 */
void Buffer::flushRange(vk::DeviceSize start, vk::DeviceSize size) {
    vmaFlushAllocation(allocator, allocation, start, size);
}

// DivisibleBuffer
DivisibleBuffer::DivisibleBuffer()
    : Buffer() {}

DivisibleBuffer::DivisibleBuffer(
    VmaAllocator allocator, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryUsage targetUsage
)
    : DivisibleBuffer() {
    allocate(allocator, size, usage, targetUsage);
}

DivisibleBuffer::~DivisibleBuffer() {
    destroy();
    freeSpaceTracking.clear();
}

void DivisibleBuffer::allocate(
    VmaAllocator allocator, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryUsage targetUsage
) {
    Buffer::allocate(allocator, size, usage, targetUsage);

    freeSpaceTracking.clear();
    freeSpaceTracking.push_back({ 0, size });
}

void DivisibleBuffer::freeSection(vk::DeviceSize offset, vk::DeviceSize size) {
    auto it = freeSpaceTracking.begin();
    for (; it != freeSpaceTracking.end(); ++it) {
        if (it->offset + it->size == offset) {
            // Contiguous after, possibly joined to free space after
            it->size += size;
            auto &lastSpace = *it;

            // Check for merges
            ++it;
            if (it->offset == lastSpace.offset + lastSpace.size) {
                // It is contiguous
                lastSpace.size += it->size;
                freeSpaceTracking.erase(it);
            }

            return;
        } else if (offset + size == it->offset) {
            // Contiguous before, but not joined to free space before
            it->offset = offset;
            it->size += size;

            return;
        } else if (it->offset > offset) {
            // Disjoint region
            freeSpaceTracking.insert(it, { offset, size });

            return;
        }

    }

    // Disjoint region at end
    freeSpaceTracking.push_back({ offset, size });
}

template<typename T>
void DivisibleBuffer::freeSection(vk::DeviceSize offset) {
    freeSection(offset, sizeof(T));
}

/**
 * Allocates a new region of the buffer.
 * @returns The offset in the buffer. Returns ALLOCATION_FAILED if allocation failed
 */
vk::DeviceSize DivisibleBuffer::allocateSection(vk::DeviceSize size) {
    // Use greedy method first. At some point we can try a best fit approach
    auto it = freeSpaceTracking.begin();
    for (; it != freeSpaceTracking.end(); ++it) {
        if (it->size >= size) {
            vk::DeviceSize offset = it->offset;

            if (it->size == size) {
                freeSpaceTracking.erase(it);
            } else {
                it->size -= size;
                it->offset += size;
            }

            return offset;
        }
    }

    // Failed to allocate
    return ALLOCATION_FAILED;
}

template<typename T>
vk::DeviceSize DivisibleBuffer::allocateSection() {
    return allocateSection(sizeof(T));
}

}