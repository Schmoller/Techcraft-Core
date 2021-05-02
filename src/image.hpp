#ifndef IMAGE_HPP
#define IMAGE_HPP


#include <vulkan/vulkan.hpp>
#include "vk_mem_alloc.h"
#include "buffer.hpp"

namespace Engine {

enum ImageLoadState {
    Unallocated = 0,
    Allocated,
    PreTransfer,
    Ready
};

enum MipType {
    NoMipmap = 0,
    StoredStandard,
    Generate
};

class Image {
    public:
    Image();
    
    ~Image();

    void allocate(VmaAllocator allocator, vk::Device device, uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, VmaMemoryUsage memUsage, vk::SampleCountFlags samples, uint32_t mipLevels);
    void destroy();

    void transfer(vk::CommandBuffer commandBuffer, void *pixelData, VkDeviceSize size, MipType mipType = MipType::NoMipmap);
    void transition(vk::CommandBuffer commandBuffer, vk::ImageLayout layout);

    void completeTransfer();

    const vk::Image image() const {
        return internalImage;
    }

    const vk::ImageView imageView() const {
        return internalImageView;
    }

    private:
    ImageLoadState state;
    VmaAllocator allocator;
    vk::Device device;

    uint32_t width;
    uint32_t height;
    vk::Format format;
    vk::Image internalImage;
    VmaAllocation imageMemory;
    vk::ImageView internalImageView;

    Buffer stagingBuffer;

    // Tracked for transitions
    vk::ImageLayout currentLayout;
};

}

#endif