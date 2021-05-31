#include <tech-core/texturemanager.hpp>
#include <stb_image.h>
#include "tech-core/image.hpp"
#include "tech-core/buffer.hpp"
#include "tech-core/engine.hpp"
#include "tech-core/task.hpp"
#include "tech-core/device.hpp"
#include "vulkanutils.hpp"

namespace Engine {

Image::Image()
    : state(ImageLoadState::Unallocated) {}

Image::~Image() {
    destroy();
}

void Image::allocate(
    VmaAllocator allocator, vk::Device device, uint32_t width, uint32_t height, vk::Format format,
    vk::ImageTiling tiling, vk::ImageUsageFlags usage, VmaMemoryUsage memUsage, vk::SampleCountFlags samples,
    uint32_t mipLevels, vk::PipelineStageFlags destinationStage
) {
    VkImageCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.extent.width = width;
    createInfo.extent.height = height;
    createInfo.extent.depth = 1;
    createInfo.mipLevels = mipLevels;
    createInfo.arrayLayers = 1;
    createInfo.format = static_cast<VkFormat>(format);
    createInfo.tiling = static_cast<VkImageTiling>(tiling);
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    createInfo.usage = static_cast<VkImageUsageFlags>(usage);
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.samples = static_cast<VkSampleCountFlagBits>(static_cast<VkSampleCountFlags>(samples));
    createInfo.flags = 0;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = memUsage;
    VkImage tempImage;

    auto result = vmaCreateImage(allocator, &createInfo, &allocInfo, &tempImage, &imageMemory, nullptr);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create texture image");
    }
    internalImage = vk::Image(tempImage);

    vk::ImageAspectFlags aspectMask;

    if ((usage & vk::ImageUsageFlagBits::eDepthStencilAttachment) ==
        static_cast<vk::ImageUsageFlags>(vk::ImageUsageFlagBits::eDepthStencilAttachment)) {
        aspectMask = vk::ImageAspectFlagBits::eDepth;
    } else {
        aspectMask = vk::ImageAspectFlagBits::eColor;
    }

    vk::ImageViewCreateInfo viewInfo(
        {},
        internalImage,
        vk::ImageViewType::e2D,
        format,
        {},
        { aspectMask, 0, mipLevels, 0, 1 }
    );

    internalImageView = device.createImageView(viewInfo);

    state = ImageLoadState::Allocated;
    this->width = width;
    this->height = height;
    this->allocator = allocator;
    this->device = device;
    this->format = format;
    this->currentLayout = vk::ImageLayout::eUndefined;
    this->destinationStage = destinationStage;
}

void Image::destroy() {
    if (allocator != VK_NULL_HANDLE && imageMemory != VK_NULL_HANDLE) {
        device.destroyImageView(internalImageView);

        vmaDestroyImage(allocator, internalImage, imageMemory);
        allocator = VK_NULL_HANDLE;
        imageMemory = VK_NULL_HANDLE;
    }
}

void Image::transfer(vk::CommandBuffer commandBuffer, void *pixelData, VkDeviceSize size, MipType mipType) {
    // Stage data for transfer
    stagingBuffer.allocate(allocator, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryUsage::eCPUOnly);
    stagingBuffer.copyIn(pixelData, size);

    // TODO: Handle mipmaps later
    if (mipType != MipType::NoMipmap) {
        throw std::runtime_error("TODO: Handle mipmaps");
    }

    vk::BufferImageCopy region(
        0,
        0,
        0,
        { vk::ImageAspectFlagBits::eColor, 0, 0, 1 },
        {},
        { width, height, 1 }
    );

    commandBuffer.copyBufferToImage(
        stagingBuffer.buffer(), internalImage, vk::ImageLayout::eTransferDstOptimal, 1, &region
    );

    state = ImageLoadState::PreTransfer;
}

void Image::completeTransfer() {
    if (state == ImageLoadState::PreTransfer) {
        stagingBuffer.destroy();
        state = ImageLoadState::Ready;
    }
}

void
Image::transition(vk::CommandBuffer commandBuffer, vk::ImageLayout layout, bool read, vk::PipelineStageFlagBits stage) {
    vk::ImageAspectFlags aspectMask;
    vk::AccessFlags srcAccessMask;
    vk::AccessFlags dstAccessMask;
    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destStage;

    if (layout == currentLayout && read == !previousWasWriting) {
        // no-op
        return;
    }

    if (layout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
        aspectMask = vk::ImageAspectFlagBits::eDepth;

        if (hasStencilComponent(format)) {
            aspectMask |= vk::ImageAspectFlagBits::eStencil;
        }
    } else {
        aspectMask = vk::ImageAspectFlagBits::eColor;
    }

    switch (layout) {
        case vk::ImageLayout::eTransferDstOptimal:
            // Original layout is ignored
            dstAccessMask = vk::AccessFlagBits::eTransferWrite;

            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destStage = vk::PipelineStageFlagBits::eTransfer;
            break;
        case vk::ImageLayout::eShaderReadOnlyOptimal: {
            dstAccessMask = vk::AccessFlagBits::eShaderRead;
            destStage = destinationStage;

            switch (currentLayout) {
                case vk::ImageLayout::eTransferDstOptimal:
                    srcAccessMask = vk::AccessFlagBits::eTransferWrite;
                    sourceStage = vk::PipelineStageFlagBits::eTransfer;
                    break;
                default:
                    if (previousWasWriting) {
                        srcAccessMask = vk::AccessFlagBits::eShaderWrite;
                    } else {
                        srcAccessMask = vk::AccessFlagBits::eShaderRead;
                    }
                    sourceStage = previousStages;
                    break;
            }
            break;
        }
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            dstAccessMask = (
                vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite
            );

            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
            break;
        case vk::ImageLayout::eGeneral:
            if (currentLayout == vk::ImageLayout::eUndefined) {
                srcAccessMask = {};
                sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            } else {
                if (previousWasWriting) {
                    srcAccessMask = vk::AccessFlagBits::eShaderWrite;
                } else {
                    srcAccessMask = vk::AccessFlagBits::eShaderRead;
                }
                sourceStage = previousStages;
            }

            if (read) {
                dstAccessMask = vk::AccessFlagBits::eShaderRead;
            } else {
                dstAccessMask = vk::AccessFlagBits::eShaderWrite;
            }

            destStage = stage;
            break;
        default:
            throw std::runtime_error("unsupported layout transition");
    }

    vk::ImageMemoryBarrier barrier(
        srcAccessMask, dstAccessMask,
        currentLayout, layout,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        internalImage,
        { aspectMask, 0, 1, 0, 1 }
    );

    commandBuffer.
        pipelineBarrier(
        sourceStage, destStage,
        {
        },
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    currentLayout = layout;
    previousStages = destStage;
    previousWasWriting = !read;
}

ImageBuilder::ImageBuilder(VulkanDevice &device, uint32_t width, uint32_t height) : device(device), width(width),
    height(height) {

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

ImageBuilder &ImageBuilder::withSampleCount(const vk::SampleCountFlags &flags) {
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
    auto image = std::make_shared<Image>();
    image->allocate(
        device.allocator, device.device, width, height, imageFormat, imageTiling,
        usageFlags, static_cast<VmaMemoryUsage>(memoryUsage),
        sampleCount, mipLevels
    );

    return image;
}


}