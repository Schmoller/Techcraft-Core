#ifndef IMAGE_HPP
#define IMAGE_HPP

#include "forward.hpp"
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
    std::shared_ptr<Image> build();

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

// TODO: Allow this class to track its usages in the pipeline:
/*
 * This means we should be able to check that it's been written to in a compute shader
 * and been read from in a fragment shader.
 * We can then use this to automatically transition the image
 *
 * Need a transitionForPipeline() function which takes the pipeline and command buffer.
 * Needs to only transition if it needs to. Eg. If it is used in a compute shader
 * and vertex shader, but the compute shader is not used often then it should
 * only transition when it is used.
 *
 * It also needs to handle queue transfer. This means, if the compute shader is run in a compute dedicated queue,
 * it would need to do a release in the graphics queue, then an aquire in the compute queue, and vice versa.
 *
 * To do this we need to know that it will be needed in the other state. ie. if the compute shader is going to be run,
 * then we must do a release at the end of the graphics pipeline. If the graphics one will be run, then it needs to
 * do a release at the end of the compute pipeline.
 * It might be possible to prepare the release ahead of time because it should respect the srcAccessMask and srcStage.
 *
 * As an interim step, we could just make sure we use shared mode instead of exclusive mode.
 *
 */
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

    void transition(
        vk::CommandBuffer commandBuffer, vk::ImageLayout layout, bool read = true,
        vk::PipelineStageFlagBits destStage = vk::PipelineStageFlagBits::eFragmentShader
    );

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
    vk::ImageLayout currentLayout { vk::ImageLayout::eUndefined };
    bool previousWasWriting { false };
    vk::PipelineStageFlags previousStages;
};

}

#endif