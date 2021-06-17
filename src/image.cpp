#include "tech-core/image.hpp"
#include "tech-core/buffer.hpp"
#include "tech-core/engine.hpp"
#include "tech-core/task.hpp"
#include "tech-core/device.hpp"
#include "vulkanutils.hpp"

namespace Engine {

Image::Image(
    VulkanDevice &device, vk::Image image, VmaAllocation memory, vk::ImageView imageView, uint32_t width,
    uint32_t height, uint32_t layers, uint32_t mipLevels, vk::Format format, const vk::PipelineStageFlags &destination
) : device(device),
    internalImage(image),
    imageMemory(memory),
    internalImageView(imageView),
    width(width),
    height(height),
    layers(layers),
    mipLevels(mipLevels),
    format(format),
    destinationStage(destination),
    rawDevice(device.device) {

    assert(width > 0);
    assert(height > 0);
    assert(layers > 0);
    assert(mipLevels > 0);
    layerStates.resize(layers);
}

Image::~Image() {
    device.device.destroyImageView(internalImageView);
    vmaDestroyImage(device.allocator, internalImage, imageMemory);
}

void Image::transferIn(vk::CommandBuffer commandBuffer, const Buffer &source, uint32_t layer, uint32_t mipLevel) {
    transferInOffset(commandBuffer, source, 0, {}, { width, height }, layer, mipLevel);
}

void Image::transferIn(
    vk::CommandBuffer commandBuffer, const Buffer &source, vk::Offset2D offset, vk::Extent2D extent, uint32_t layer,
    uint32_t mipLevel
) {
    transferInOffset(commandBuffer, source, 0, offset, extent, layer, mipLevel);
}

void Image::transferInOffset(
    vk::CommandBuffer commandBuffer, const Buffer &source, vk::DeviceSize bufferOffset, uint32_t layer,
    uint32_t mipLevel
) {
    transferInOffset(commandBuffer, source, bufferOffset, {}, { width, height }, layer, mipLevel);
}

void Image::transferInOffset(
    vk::CommandBuffer commandBuffer, const Buffer &source, vk::DeviceSize bufferOffset, vk::Offset2D offset,
    vk::Extent2D extent, uint32_t layer, uint32_t mipLevel
) {
    vk::BufferImageCopy region(
        bufferOffset,
        0,
        0,
        { vk::ImageAspectFlagBits::eColor, mipLevel, layer, 1 },
        { offset.x, offset.y, 0 },
        { extent.width, extent.height, 1 }
    );

    commandBuffer.copyBufferToImage(
        source.buffer(), internalImage, vk::ImageLayout::eTransferDstOptimal, 1, &region
    );
}

void Image::transferOut(vk::CommandBuffer commandBuffer, Buffer *buffer, uint32_t layer) {
    vk::BufferImageCopy region(
        0,
        0,
        0,
        { vk::ImageAspectFlagBits::eColor, 0, layer, 1 },
        {},
        { width, height, 1 }
    );

    commandBuffer.copyImageToBuffer(
        internalImage, layerStates[layer].currentLayout, buffer->buffer(), 1, &region
    );
}

void
Image::transition(vk::CommandBuffer commandBuffer, vk::ImageLayout layout, bool read, vk::PipelineStageFlagBits stage) {
    transition(commandBuffer, 0, layers, layout, read, stage);
}

void Image::transition(
    vk::CommandBuffer commandBuffer, uint32_t startLayer, uint32_t layerCount, vk::ImageLayout layout, bool read,
    vk::PipelineStageFlagBits destStage
) {
    vk::PipelineStageFlags sourceStages;
    vk::PipelineStageFlags destStages;

    std::vector<vk::ImageMemoryBarrier> barriers(layerCount);

    vk::ImageAspectFlags aspectMask;
    if (layout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        aspectMask = vk::ImageAspectFlagBits::eDepth;

        if (hasStencilComponent(format)) {
            aspectMask |= vk::ImageAspectFlagBits::eStencil;
        }
    } else {
        aspectMask = vk::ImageAspectFlagBits::eColor;
    }

    for (uint32_t layer = startLayer; layer < startLayer + layerCount; ++layer) {
        vk::AccessFlags srcAccessMask;
        vk::AccessFlags dstAccessMask;
        vk::PipelineStageFlags layerDestStage;

        auto &state = layerStates[layer];

        switch (layout) {
            case vk::ImageLayout::eTransferDstOptimal:
                // Original layout is ignored
                dstAccessMask = vk::AccessFlagBits::eTransferWrite;

                sourceStages |= vk::PipelineStageFlagBits::eTopOfPipe;
                layerDestStage = vk::PipelineStageFlagBits::eTransfer;
                break;
            case vk::ImageLayout::eShaderReadOnlyOptimal: {
                dstAccessMask = vk::AccessFlagBits::eShaderRead;
                layerDestStage = destinationStage;

                switch (state.currentLayout) {
                    case vk::ImageLayout::eTransferDstOptimal:
                        srcAccessMask = vk::AccessFlagBits::eTransferWrite;
                        sourceStages |= vk::PipelineStageFlagBits::eTransfer;
                        break;
                    case vk::ImageLayout::eTransferSrcOptimal:
                        srcAccessMask = vk::AccessFlagBits::eTransferRead;
                        sourceStages |= vk::PipelineStageFlagBits::eTransfer;
                        break;
                    case vk::ImageLayout::eUndefined:
                        sourceStages |= vk::PipelineStageFlagBits::eTopOfPipe;
                        break;
                    default:
                        if (state.previousWasWriting) {
                            srcAccessMask = vk::AccessFlagBits::eShaderWrite;
                        } else {
                            srcAccessMask = vk::AccessFlagBits::eShaderRead;
                        }
                        sourceStages |= state.previousStages;
                        break;
                }
                break;
            }
            case vk::ImageLayout::eDepthStencilAttachmentOptimal:
                dstAccessMask = (
                    vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite
                );

                sourceStages |= vk::PipelineStageFlagBits::eTopOfPipe;
                layerDestStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
                break;
            case vk::ImageLayout::eGeneral:
                if (state.currentLayout == vk::ImageLayout::eUndefined) {
                    srcAccessMask = {};
                    sourceStages |= vk::PipelineStageFlagBits::eTopOfPipe;
                } else if (state.currentLayout == vk::ImageLayout::eTransferDstOptimal) {
                    srcAccessMask = vk::AccessFlagBits::eTransferWrite;
                    sourceStages |= vk::PipelineStageFlagBits::eTransfer;
                } else {
                    if (state.previousWasWriting) {
                        srcAccessMask = vk::AccessFlagBits::eShaderWrite;
                    } else {
                        srcAccessMask = vk::AccessFlagBits::eShaderRead;
                    }
                    sourceStages |= state.previousStages;
                }

                if (read) {
                    dstAccessMask = vk::AccessFlagBits::eShaderRead;
                } else {
                    dstAccessMask = vk::AccessFlagBits::eShaderWrite;
                }
                layerDestStage = destStage;
                break;
            default:
                throw std::runtime_error("unsupported layout transition");
        }

        barriers[layer - startLayer] = vk::ImageMemoryBarrier(
            srcAccessMask, dstAccessMask,
            state.currentLayout, layout,
            VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
            internalImage,
            { aspectMask, 0, mipLevels, layer, 1 }
        );

        state.currentLayout = layout;
        state.previousStages = layerDestStage;
        state.previousWasWriting = !read;

        destStages |= layerDestStage;
    }

    assert(sourceStages);
    assert(destStages);
    commandBuffer.pipelineBarrier(
        sourceStages, destStages,
        {},
        0, nullptr,
        0, nullptr,
        vkUseArray(barriers)
    );
}

vk::AccessFlags getAccessFrom(vk::ImageLayout layout, bool read) {
    switch (layout) {
        default:
        case vk::ImageLayout::eUndefined:
            return {};
        case vk::ImageLayout::eTransferDstOptimal:
            if (read) {
                return {};
            } else {
                return vk::AccessFlagBits::eTransferWrite;
            }
        case vk::ImageLayout::eGeneral:
        case vk::ImageLayout::eDepthReadOnlyOptimal:
        case vk::ImageLayout::eStencilReadOnlyOptimal:
            if (read) {
                return vk::AccessFlagBits::eShaderRead;
            } else {
                return vk::AccessFlagBits::eShaderWrite;
            }
        case vk::ImageLayout::eColorAttachmentOptimal:
            if (read) {
                return vk::AccessFlagBits::eColorAttachmentRead;
            } else {
                return vk::AccessFlagBits::eColorAttachmentWrite;
            }
        case vk::ImageLayout::eDepthAttachmentOptimal:
        case vk::ImageLayout::eStencilAttachmentOptimal:
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            if (read) {
                return vk::AccessFlagBits::eDepthStencilAttachmentRead;
            } else {
                return vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            }
        case vk::ImageLayout::eDepthStencilReadOnlyOptimal:
        case vk::ImageLayout::eShaderReadOnlyOptimal:
            return vk::AccessFlagBits::eShaderRead;
        case vk::ImageLayout::eTransferSrcOptimal:
            return vk::AccessFlagBits::eTransferRead;
        case vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal:
        case vk::ImageLayout::eDepthAttachmentStencilReadOnlyOptimal:
            return vk::AccessFlagBits::eDepthStencilAttachmentRead;

        case vk::ImageLayout::ePresentSrcKHR:
        case vk::ImageLayout::eSharedPresentKHR:
            return vk::AccessFlagBits::eColorAttachmentWrite;
    }
}

void Image::transitionManual(
    vk::CommandBuffer commandBuffer, uint32_t layer, uint32_t layerCount, uint32_t mipLevel, uint32_t levelCount,
    vk::ImageLayout oldLayout, bool wasWritten, vk::ImageLayout newLayout, bool willWrite,
    const vk::PipelineStageFlags &sourceStages, const vk::PipelineStageFlags &destStages
) {
    vk::AccessFlags sourceAccess;
    vk::AccessFlags destAccess;

    vk::ImageAspectFlags aspectMask;
    if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        aspectMask = vk::ImageAspectFlagBits::eDepth;

        if (hasStencilComponent(format)) {
            aspectMask |= vk::ImageAspectFlagBits::eStencil;
        }
    } else {
        aspectMask = vk::ImageAspectFlagBits::eColor;
    }

    if (newLayout == vk::ImageLayout::eTransferDstOptimal) {
        destAccess = vk::AccessFlagBits::eTransferWrite;
        // Ignore the previous layout since it will be overridden
        sourceAccess = {};
        oldLayout = vk::ImageLayout::eUndefined;
    } else {
        sourceAccess = getAccessFrom(oldLayout, wasWritten);
        destAccess = getAccessFrom(newLayout, willWrite);
    }

    vk::ImageMemoryBarrier barrier(
        sourceAccess, destAccess,
        oldLayout, newLayout,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        internalImage,
        { aspectMask, mipLevel, levelCount, layer, layerCount }
    );

    assert(sourceStages);
    assert(destStages);
    commandBuffer.pipelineBarrier(
        sourceStages, destStages,
        {},
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
}

void Image::transitionOverride(
    vk::ImageLayout layout, bool didWrite, const vk::PipelineStageFlags &previousStage, uint32_t layer
) {
    layerStates[layer] = { layout, didWrite, previousStage };
}

Image::operator ImTextureID() const {
    return reinterpret_cast<ImTextureID>(const_cast<Image *>(this));
}

bool Image::isImage(ImTextureID id, vk::Device device) {
    auto image = reinterpret_cast<Image *>(id);
    if (image->rawDevice != device) {
        return false;
    }

    return true;
}

bool Image::isReadyForSampling() const {
    for (uint32_t layer = 0; layer < layers; ++layer) {
        auto layout = layerStates[layer].currentLayout;
        if (layout != vk::ImageLayout::eShaderReadOnlyOptimal && layout != vk::ImageLayout::eGeneral) {
            return false;
        }
    }

    return true;
}

ImageBuilder::ImageBuilder(VulkanDevice &device, uint32_t width, uint32_t height) : device(device), width(width),
    height(height) {

}


ImageBuilder::ImageBuilder(VulkanDevice &device, uint32_t width, uint32_t height, uint32_t count) : device(device),
    width(width),
    height(height), arrayLayers(count) {

}

ImageBuilder &ImageBuilder::withUsage(const vk::ImageUsageFlags &flags) {
    usageFlags = flags;
    return *this;
}

ImageBuilder &ImageBuilder::withImageTiling(vk::ImageTiling tiling) {
    imageTiling = tiling;
    return *this;
}

ImageBuilder &ImageBuilder::withFormat(vk::Format format) {
    imageFormat = format;
    return *this;
}

ImageBuilder &ImageBuilder::withSampleCount(const vk::SampleCountFlagBits &flags) {
    sampleCount = flags;
    return *this;
}

ImageBuilder &ImageBuilder::withMemoryUsage(vk::MemoryUsage usage) {
    memoryUsage = usage;
    return *this;
}

ImageBuilder &ImageBuilder::withMipLevels(uint32_t levels) {
    if (levels >= 1) {
        mipLevels = levels;
    }
    return *this;
}

ImageBuilder &ImageBuilder::withDestinationStage(const vk::PipelineStageFlags &flags) {
    destinationStage = flags;
    return *this;
}

std::shared_ptr<Image> ImageBuilder::build() {
    vk::ImageCreateInfo createInfo(
        {},
        vk::ImageType::e2D,
        imageFormat,
        { width, height, 1 },
        mipLevels,
        arrayLayers,
        sampleCount,
        imageTiling,
        usageFlags
    );

    auto createInfoC = static_cast<VkImageCreateInfo>(createInfo);
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = static_cast<VmaMemoryUsage>(memoryUsage);
    VkImage tempImage;
    VmaAllocation imageMemory;
    vk::ImageView internalImageView;

    auto result = vmaCreateImage(device.allocator, &createInfoC, &allocInfo, &tempImage, &imageMemory, nullptr);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create texture image");
    }

    vk::ImageAspectFlags aspectMask;

    if ((usageFlags & vk::ImageUsageFlagBits::eDepthStencilAttachment) ==
        static_cast<vk::ImageUsageFlags>(vk::ImageUsageFlagBits::eDepthStencilAttachment)) {
        aspectMask = vk::ImageAspectFlagBits::eDepth;
    } else {
        aspectMask = vk::ImageAspectFlagBits::eColor;
    }

    vk::ImageViewCreateInfo viewInfo(
        {},
        tempImage,
        arrayLayers == 1 ? vk::ImageViewType::e2D : vk::ImageViewType::e2DArray,
        imageFormat,
        {},
        { aspectMask, 0, mipLevels, 0, arrayLayers }
    );

    internalImageView = device.device.createImageView(viewInfo);

    auto image = std::shared_ptr<Image>(
        new Image(
            device, tempImage, imageMemory, internalImageView, width, height, arrayLayers, mipLevels, imageFormat,
            destinationStage
        )
    );

    return image;
}


}
