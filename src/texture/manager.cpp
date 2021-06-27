#include "tech-core/texture/manager.hpp"
#include "tech-core/texture/builder.hpp"
#include "tech-core/engine.hpp"
#include "tech-core/device.hpp"
#include "tech-core/image.hpp"
#include "../imageutils.hpp"
#include "sampler_cache.hpp"
#include <iostream>
#include <cmath>

#define STB_IMAGE_RESIZE_IMPLEMENTATION

#include <stb_image_resize.h>

namespace Engine {

TextureManager::TextureManager(RenderEngine &engine, VulkanDevice &device, vk::PhysicalDevice physicalDevice)
    : engine(engine), device(device) {

    samplers = std::make_shared<Internal::SamplerCache>(device);

    // See if we can blit textures
    auto properties = physicalDevice.getFormatProperties(vk::Format::eR8G8B8A8Unorm);
    if (properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitSrc &&
        properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eBlitDst) {
        canBlitTextures = true;
    }

    auto deviceProperties = physicalDevice.getProperties();
    maxAnisotropy = deviceProperties.limits.maxSamplerAnisotropy;

    generatePlaceholders();
}

TextureBuilder TextureManager::add(const std::string &name) {
    return TextureBuilder(*this, name);
}

const Texture *TextureManager::get(const std::string &name) const {
    auto it = texturesByName.find(name);
    if (it == texturesByName.end()) {
        return errorTexture;
    }

    return it->second.get();
}

bool TextureManager::remove(const std::string &name) {
    auto it = texturesByName.find(name);
    if (it == texturesByName.end()) {
        return false;
    }

    texturesByName.erase(it);
    return true;
}

const Texture *TextureManager::add(const TextureBuilder &builder) {
    bool useFallbackMipmapGen = !canBlitTextures;

    uint32_t width = builder.width;
    uint32_t height = builder.height;
    uint32_t srcWidth = width;
    void *pixelData = builder.pixelData;

    if (builder.mipType == TextureMipType::StoredStandard) {
        // Standard mipmaps are stored on the right of the main texture using 50% more width
        width *= 2.0 / 3.0;
    }

    vk::DeviceSize imageSize = srcWidth * height * 4;
    uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

    unsigned char *combinedPixels = nullptr;
    if (builder.mipType == TextureMipType::Generate && useFallbackMipmapGen) {
        combinedPixels = generateMipMapsFallback(srcWidth, height, mipLevels, pixelData, &imageSize);
        pixelData = combinedPixels;
    }

    auto stagingBuffer = engine.getBufferManager().aquireStaging(imageSize);
    stagingBuffer->copyIn(pixelData);

    auto usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
    if (builder.mipType == TextureMipType::Generate && !useFallbackMipmapGen) {
        usage |= vk::ImageUsageFlagBits::eTransferSrc;
    }

    auto task = engine.getTaskManager().createTask();
    auto imageBuilder = engine.createImage(width, height)
        .withFormat(vk::Format::eR8G8B8A8Unorm)
        .withImageTiling(vk::ImageTiling::eOptimal)
        .withUsage(usage)
        .withMemoryUsage(vk::MemoryUsage::eGPUOnly)
        .withDestinationStage(vk::PipelineStageFlagBits::eFragmentShader);

    if (builder.mipType != TextureMipType::None) {
        imageBuilder.withMipLevels(mipLevels);
    }

    auto image = imageBuilder.build();

    task->execute(
        [&stagingBuffer, image, mipLevels, builder, useFallbackMipmapGen](vk::CommandBuffer buffer) {
            image->transition(
                buffer, vk::ImageLayout::eTransferDstOptimal, false, vk::PipelineStageFlagBits::eTransfer
            );

            uint32_t width = image->getWidth();
            uint32_t height = image->getHeight();

            if (builder.mipType == TextureMipType::None ||
                (builder.mipType == TextureMipType::Generate && !useFallbackMipmapGen)) {
                image->transferIn(buffer, *stagingBuffer);
            } else if (builder.mipType == TextureMipType::StoredStandard) {
                // Standard storage:
                /*
                +-----+---+
                |     |   |
                |     +-+-+
                |     +-+
                +-----+.
                */

                uint32_t offsetY = 0;

                uint32_t mipHeight = height;

                for (uint32_t level = 0; level < mipLevels; ++level) {
                    if (level == 0) {
                        // Base Level
                        image->transferIn(buffer, *stagingBuffer, 0, 0);
                    } else {
                        // Mip Levels
                        if (mipHeight > 1) {
                            mipHeight >>= 1;
                        }

                        image->transferIn(
                            buffer,
                            *stagingBuffer,
                            { static_cast<int32_t>(width), static_cast<int32_t>(offsetY) },
                            { std::max(width >> level, 1U), std::max(height >> level, 1U) },
                            0,
                            level
                        );

                        offsetY += mipHeight;
                    }
                }
            } else if (builder.mipType == TextureMipType::Generate && useFallbackMipmapGen) {
                size_t offset = width * height * 4;

                for (uint32_t level = 0; level < mipLevels; ++level) {
                    if (level == 0) {
                        // Base Level
                        image->transferIn(buffer, *stagingBuffer, 0, 0);
                    } else {
                        uint32_t mipWidth = width >> level;
                        uint32_t mipHeight = height >> level;
                        auto size = mipWidth * mipHeight * 4;

                        image->transferInOffset(
                            buffer, *stagingBuffer, offset, {}, { mipWidth, mipHeight }, 0, level
                        );
                        offset += size;
                    }
                }
            }

            if (builder.mipType == TextureMipType::Generate && !useFallbackMipmapGen) {
                // Blit the mipmaps
                generateMipmaps(buffer, image);
            }

            image->transition(
                buffer, vk::ImageLayout::eShaderReadOnlyOptimal, false,
                vk::PipelineStageFlagBits::eFragmentShader
            );
        }
    );

    task->freeWhenDone(std::move(stagingBuffer));

    engine.getTaskManager().submitTask(std::move(task));

    delete[] combinedPixels;

    // Make sampler
    auto sampler = samplers->acquire(
        {
            builder.filtering,
            builder.mipType != TextureMipType::None,
            builder.wrapU,
            builder.wrapV,
            builder.anisotropy
        }
    );

    auto texture = std::make_shared<Texture>(builder.name, image, sampler);
    texturesByName[builder.name] = texture;

    std::cout << "Loaded texture " << texture->getName() << std::endl;

    return texture.get();
}

unsigned char *TextureManager::generateMipMapsFallback(
    uint32_t width, uint32_t height, uint32_t mipLevels, void *source, vk::DeviceSize *outputSize
) {
    vk::DeviceSize imageSize = width * height * 4;

    // Add each successive mip layer directly after the last
    for (uint32_t level = 1; level < mipLevels; ++level) {
        uint32_t factor = 1 << (level - 1);
        uint32_t mipWidth = width >> factor;
        uint32_t mipHeight = height >> factor;

        imageSize += mipWidth * mipHeight * 4;
    }

    auto combinedPixels = new unsigned char[imageSize];
    std::memcpy(combinedPixels, source, width * height * 4);
    size_t offset = width * height * 4;

    // Generate Mips
    for (uint32_t level = 1; level < mipLevels; ++level) {
        uint32_t mipWidth = width >> level;
        uint32_t mipHeight = height >> level;

        auto result = stbir_resize_uint8(
            reinterpret_cast<unsigned char *>(source),
            width, height,
            0,
            combinedPixels + offset,
            mipWidth, mipHeight,
            0,
            4
        );
        if (!result) {
            throw std::runtime_error("Failed to generate mip level");
        }

        offset += mipWidth * mipHeight * 4;
    }

    *outputSize = imageSize;
    return combinedPixels;
}

void TextureManager::generatePlaceholders() {
    VkDeviceSize imageSize = PLACEHOLDER_TEXTURE_SIZE * PLACEHOLDER_TEXTURE_SIZE;

    auto *pixels = new uint32_t[imageSize];
    generateErrorPixels(PLACEHOLDER_TEXTURE_SIZE, PLACEHOLDER_TEXTURE_SIZE, pixels);

    errorTexture = add("internal.error")
        .fromRaw(PLACEHOLDER_TEXTURE_SIZE, PLACEHOLDER_TEXTURE_SIZE, pixels)
        .finish();

    generateSolidPixels(PLACEHOLDER_TEXTURE_SIZE, PLACEHOLDER_TEXTURE_SIZE, pixels, 0x00000000);

    transparentTexture = add("internal.loading")
        .fromRaw(PLACEHOLDER_TEXTURE_SIZE, PLACEHOLDER_TEXTURE_SIZE, pixels)
        .finish();

    generateSolidPixels(PLACEHOLDER_TEXTURE_SIZE, PLACEHOLDER_TEXTURE_SIZE, pixels, 0xFFFFFFFF);

    whiteTexture = add("internal.white")
        .fromRaw(PLACEHOLDER_TEXTURE_SIZE, PLACEHOLDER_TEXTURE_SIZE, pixels)
        .finish();

    delete[] pixels;
}

void TextureManager::generateMipmaps(vk::CommandBuffer buffer, const std::shared_ptr<Image> &image) {
    // NOTE: It is expected that the image is currently in the TransferDstOptimal layout

    for (uint32_t level = 1; level < image->getMipLevels(); ++level) {
        // Prepare the previous level to transfer to this level
        image->transitionManual(
            buffer,
            0, 1,
            level - 1, 1,
            vk::ImageLayout::eTransferDstOptimal, true,
            vk::ImageLayout::eTransferSrcOptimal, false,
            vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer
        );

        int32_t srcWidth = static_cast<int32_t>(image->getWidth()) >> (level - 1);
        int32_t srcHeight = static_cast<int32_t>(image->getHeight()) >> (level - 1);

        std::array<vk::Offset3D, 2> sourceCoords {{
            {},
            { srcWidth, srcHeight, 1 }
        }};
        std::array<vk::Offset3D, 2> destCoords {{
            {},
            { srcWidth >> 1, srcHeight >> 1, 1 }
        }};

        vk::ImageBlit blit(
            { vk::ImageAspectFlagBits::eColor, level - 1, 0, 1 },
            sourceCoords,
            { vk::ImageAspectFlagBits::eColor, level, 0, 1 },
            destCoords
        );

        buffer.blitImage(
            image->image(), vk::ImageLayout::eTransferSrcOptimal,
            image->image(), vk::ImageLayout::eTransferDstOptimal,
            1, &blit,
            vk::Filter::eLinear
        );
    }

    image->transitionManual(
        buffer,
        0, 1,
        image->getMipLevels() - 1, 1,
        vk::ImageLayout::eTransferDstOptimal, true,
        vk::ImageLayout::eTransferSrcOptimal, false,
        vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer
    );

    image->transitionOverride(vk::ImageLayout::eTransferSrcOptimal, true, vk::PipelineStageFlagBits::eTransfer);
}

}
