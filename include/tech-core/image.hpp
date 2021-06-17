#ifndef IMAGE_HPP
#define IMAGE_HPP

#include "forward.hpp"
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include "tech-core/buffer.hpp"

// Forward def
typedef void *ImTextureID;

namespace Engine {

class ImageBuilder {
    friend class RenderEngine;

public:
    ImageBuilder &withUsage(const vk::ImageUsageFlags &);
    ImageBuilder &withImageTiling(vk::ImageTiling);
    ImageBuilder &withFormat(vk::Format);
    ImageBuilder &withSampleCount(const vk::SampleCountFlagBits &);
    ImageBuilder &withMemoryUsage(vk::MemoryUsage);
    ImageBuilder &withMipLevels(uint32_t);
    ImageBuilder &withDestinationStage(const vk::PipelineStageFlags &);
    std::shared_ptr<Image> build();

private:
    explicit ImageBuilder(VulkanDevice &, uint32_t width, uint32_t height);
    explicit ImageBuilder(VulkanDevice &, uint32_t width, uint32_t height, uint32_t count);

    // Non-configurable
    VulkanDevice &device;

    // Configurable
    vk::Format imageFormat { vk::Format::eR8G8B8A8Unorm };
    vk::ImageUsageFlags usageFlags { vk::ImageUsageFlagBits::eSampled };
    vk::ImageTiling imageTiling { vk::ImageTiling::eOptimal };
    vk::SampleCountFlagBits sampleCount { vk::SampleCountFlagBits::e1 };
    vk::MemoryUsage memoryUsage { vk::MemoryUsage::eGPUOnly };
    vk::PipelineStageFlags destinationStage { vk::PipelineStageFlagBits::eFragmentShader };

    uint32_t mipLevels { 1 };
    uint32_t width { 0 };
    uint32_t height { 0 };
    uint32_t arrayLayers { 1 };
};

class Image {
    friend class ImageBuilder;
public:
    ~Image();

    void transferIn(vk::CommandBuffer commandBuffer, const Buffer &source, uint32_t layer = 0, uint32_t mipLevel = 0);
    void transferIn(
        vk::CommandBuffer commandBuffer, const Buffer &source, vk::Offset2D offset, vk::Extent2D extent,
        uint32_t layer = 0,
        uint32_t mipLevel = 0
    );
    void transferInOffset(
        vk::CommandBuffer commandBuffer, const Buffer &source, vk::DeviceSize bufferOffset, uint32_t layer = 0,
        uint32_t mipLevel = 0
    );
    void transferInOffset(
        vk::CommandBuffer commandBuffer, const Buffer &source, vk::DeviceSize bufferOffset, vk::Offset2D offset,
        vk::Extent2D extent, uint32_t layer = 0, uint32_t mipLevel = 0
    );

    void transferOut(vk::CommandBuffer commandBuffer, Buffer *buffer, uint32_t layer = 0);

    void transition(
        vk::CommandBuffer commandBuffer, vk::ImageLayout layout, bool read = true,
        vk::PipelineStageFlagBits destStage = vk::PipelineStageFlagBits::eFragmentShader
    );
    void transition(
        vk::CommandBuffer commandBuffer, uint32_t layer, uint32_t layerCount, vk::ImageLayout layout,
        bool read = true, vk::PipelineStageFlagBits destStage = vk::PipelineStageFlagBits::eFragmentShader
    );

    void transitionManual(
        vk::CommandBuffer commandBuffer, uint32_t layer, uint32_t layerCount, uint32_t mipLevel, uint32_t levelCount,
        vk::ImageLayout oldLayout, bool wasWritten, vk::ImageLayout newLayout, bool willWrite,
        const vk::PipelineStageFlags &srcStages, const vk::PipelineStageFlags &destStages
    );


    const vk::Image image() const {
        return internalImage;
    }

    const vk::ImageView imageView() const {
        return internalImageView;
    }

    vk::ImageLayout getCurrentLayout(uint32_t layer = 0) const { return layerStates[layer].currentLayout; }

    uint32_t getWidth() const { return width; }

    uint32_t getHeight() const { return height; }

    uint32_t getLayers() const { return layers; }

    uint32_t getMipLevels() const { return mipLevels; }

    bool isReadyForSampling() const;

    explicit operator ImTextureID() const;

    static bool isImage(ImTextureID id, vk::Device device);

private:
    // This exists just to check if a ImGui texture is an image
    vk::Device rawDevice;
    VulkanDevice &device;

    uint32_t width;
    uint32_t height;
    uint32_t layers;
    uint32_t mipLevels;
    vk::Format format;
    vk::Image internalImage;
    VmaAllocation imageMemory;
    vk::ImageView internalImageView;
    vk::PipelineStageFlags destinationStage { vk::PipelineStageFlagBits::eFragmentShader };

    // Tracked for transitions
    struct LayoutState {
        vk::ImageLayout currentLayout { vk::ImageLayout::eUndefined };
        bool previousWasWriting { false };
        vk::PipelineStageFlags previousStages {};
    };

    std::vector<LayoutState> layerStates;

    Image(
        VulkanDevice &, vk::Image, VmaAllocation memory, vk::ImageView, uint32_t width, uint32_t height,
        uint32_t layers, uint32_t mipLevels, vk::Format, const vk::PipelineStageFlags &destination
    );
};

}

#endif