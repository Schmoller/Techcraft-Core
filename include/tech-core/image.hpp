#ifndef IMAGE_HPP
#define IMAGE_HPP


#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include "tech-core/buffer.hpp"

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

class RenderEngine;

class TaskManager;

class Image;

class ImageBuilder {
    friend class RenderEngine;

public:
    ImageBuilder &withUsage(const vk::ImageUsageFlags &);

    ImageBuilder &withImageTiling(vk::ImageTiling);

    ImageBuilder &withFormat(vk::Format);

    ImageBuilder &withSampleCount(const vk::SampleCountFlags &);

    ImageBuilder &withMemoryUsage(vk::MemoryUsage);

    ImageBuilder &withMipLevels(uint32_t);

    ImageBuilder &withDestinationStage(const vk::PipelineStageFlags &);

    std::unique_ptr<Image> build();

private:
    explicit ImageBuilder(VulkanDevice &, uint32_t width, uint32_t height);

    // Non-configurable
    VulkanDevice &device;

    // Configurable
    vk::Format imageFormat { vk::Format::eR8G8B8A8Unorm };
    vk::ImageUsageFlags usageFlags { vk::ImageUsageFlagBits::eSampled };
    vk::ImageTiling imageTiling { vk::ImageTiling::eOptimal };
    vk::SampleCountFlags sampleCount { vk::SampleCountFlagBits::e1 };
    vk::MemoryUsage memoryUsage { vk::MemoryUsage::eGPUOnly };
    vk::PipelineStageFlags destinationStage { vk::PipelineStageFlagBits::eFragmentShader };

    uint32_t mipLevels { 1 };
    uint32_t width { 0 };
    uint32_t height { 0 };
};


class Image {
public:
    Image();

    ~Image();

    // TODO: remove this and make it part of the construction
    void allocate(
        VmaAllocator allocator, vk::Device device, uint32_t width, uint32_t height, vk::Format format,
        vk::ImageTiling tiling, vk::ImageUsageFlags usage, VmaMemoryUsage memUsage, vk::SampleCountFlags samples,
        uint32_t mipLevels, vk::PipelineStageFlags destinationStage = vk::PipelineStageFlagBits::eFragmentShader
    );

    void destroy();

    void
    transfer(vk::CommandBuffer commandBuffer, void *pixelData, VkDeviceSize size, MipType mipType = MipType::NoMipmap);

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
    vk::PipelineStageFlags destinationStage { vk::PipelineStageFlagBits::eFragmentShader };

    Buffer stagingBuffer;

    // Tracked for transitions
    vk::ImageLayout currentLayout;
};

}

#endif