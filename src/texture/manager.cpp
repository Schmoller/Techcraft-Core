#include "tech-core/texture/manager.hpp"
#include "tech-core/texture/builder.hpp"
#include "tech-core/engine.hpp"
#include "tech-core/device.hpp"
#include "tech-core/image.hpp"
#include "../imageutils.hpp"
#include "internal.hpp"
#include <iostream>
#include <cmath>

#define STB_IMAGE_RESIZE_IMPLEMENTATION

#include <stb_image_resize.h>

// Memory for 5 4k 32bit textures
const vk::DeviceSize MAX_MEMORY_USAGE = 335544320;

const uint32_t MAX_ARRAY_SIZE = 16;

namespace Engine {

TextureManager::TextureManager(RenderEngine &engine, VulkanDevice &device)
    : engine(engine), device(device) {

    vk::DescriptorSetLayoutBinding samplerBinding(
        TEXTURE_BINDING, vk::DescriptorType::eCombinedImageSampler,
        1, vk::ShaderStageFlagBits::eFragment
    );

    vk::DescriptorSetLayoutCreateInfo textureLayoutInfo(
        {}, 1, &samplerBinding
    );

    descriptorSetLayout = device.device.createDescriptorSetLayout(textureLayoutInfo);

    vk::DescriptorPoolSize poolSize(
        vk::DescriptorType::eCombinedImageSampler,
        DESCRIPTOR_POOL_SIZE
    );

    descriptorPool = device.device.createDescriptorPool({{}, DESCRIPTOR_POOL_SIZE, 1, &poolSize });

    generatePlaceholders();
}

TextureManager::~TextureManager() {
    device.device.destroyDescriptorPool(descriptorPool);
    device.device.destroyDescriptorSetLayout(descriptorSetLayout);
}

TextureBuilder TextureManager::createTexture(const std::string &name) {
    return TextureBuilder(*this, name);
}

const Texture *TextureManager::getTexture(const std::string &name) const {
    auto it = texturesByName.find(name);
    if (it == texturesByName.end()) {
        return errorTexture;
    }

    return &it->second;
}

bool TextureManager::removeTexture(const std::string &name) {
    return false;
}

Internal::TextureArray &TextureManager::allocateArray(uint32_t width, uint32_t height) {
    for (auto &pair : textureArrays) {
        auto &array = pair.second;

        if (array->canAllocate(width, height)) {
            return *array;
        }
    }

    // No free slots in any texture arrays, or not matching sizes
    std::cout << "Creating texture array for size " << width << "x" << height << std::endl;

    uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

    VkDeviceSize imageUsage = width * height * 4;
    uint32_t arraySize = std::min(static_cast<uint32_t>(MAX_MEMORY_USAGE / imageUsage), MAX_ARRAY_SIZE);

    auto image = engine.createImageArray(width, height, arraySize)
        .withFormat(vk::Format::eR8G8B8A8Unorm)
        .withMipLevels(mipLevels)
        .withUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
        .withMemoryUsage(vk::MemoryUsage::eGPUOnly)
        .withImageTiling(vk::ImageTiling::eOptimal)
        .build();

    auto array = std::make_shared<Internal::TextureArray>(nextArrayId++, image, mipLevels);

    auto task = engine.getTaskManager().createTask();

    // Initialize the image
    task->execute(
        [image](vk::CommandBuffer buffer) {
            image->transition(buffer, vk::ImageLayout::eShaderReadOnlyOptimal);
        }
    );

    engine.getTaskManager().submitTask(std::move(task));

    textureArrays[array->getId()] = array;
    return *array;
}

const Texture *TextureManager::addTexture(
    const std::string &name, uint32_t width, uint32_t height, void *pixelData, MipType mipType
) {
    uint32_t srcWidth = width;
    if (mipType == MipType::StoredStandard) {
        // Standard mipmaps are stored on the right of the main texture using 50% more width
        width *= 2.0 / 3.0;
    }

    // Find a place to store the texture
    auto &array = allocateArray(width, height);
    size_t slot = array.allocateSlot();

    Texture texture = {
        array.getId(),
        slot,
        mipType != MipType::NoMipmap,
        width,
        height
    };

    vk::DeviceSize imageSize = srcWidth * height * 4;

    unsigned char *combinedPixels = nullptr;
    if (mipType == MipType::Generate) {
        combinedPixels = generateMipMaps(srcWidth, height, array.getMipLevels(), pixelData, &imageSize);
        pixelData = combinedPixels;
    }

    auto stagingBuffer = engine.getBufferManager().aquireStaging(imageSize);
    stagingBuffer->copyIn(pixelData);

    auto task = engine.getTaskManager().createTask();

    task->execute(
        [&stagingBuffer, array, slot, mipType](vk::CommandBuffer buffer) {
            auto image = array.getImage();
            image->transition(
                buffer, slot, 1, vk::ImageLayout::eTransferDstOptimal, false, vk::PipelineStageFlagBits::eTransfer
            );

            uint32_t width = image->getWidth();
            uint32_t height = image->getHeight();

            if (mipType == MipType::NoMipmap) {
                image->transferIn(buffer, *stagingBuffer, slot);
            } else if (mipType == MipType::StoredStandard) {
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

                for (uint32_t level = 0; level < array.getMipLevels(); ++level) {
                    if (level == 0) {
                        // Base Level
                        image->transferIn(buffer, *stagingBuffer, slot, 0);
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
                            slot,
                            level
                        );

                        offsetY += mipHeight;
                    }
                }
            } else {
                size_t offset = width * height * 4;

                for (uint32_t level = 0; level < array.getMipLevels(); ++level) {
                    if (level == 0) {
                        // Base Level
                        image->transferIn(buffer, *stagingBuffer, slot, 0);
                    } else {
                        uint32_t mipWidth = width >> level;
                        uint32_t mipHeight = height >> level;
                        auto size = mipWidth * mipHeight * 4;

                        image->transferInOffset(
                            buffer, *stagingBuffer, offset, {}, { mipWidth, mipHeight }, slot, level
                        );
                        offset += size;
                    }
                }
            }

            image->transition(
                buffer, slot, 1, vk::ImageLayout::eShaderReadOnlyOptimal, false,
                vk::PipelineStageFlagBits::eFragmentShader
            );
        }
    );

    task->freeWhenDone(std::move(stagingBuffer));

    engine.getTaskManager().submitTask(std::move(task));

    delete[] combinedPixels;

    std::cout << "Loaded texture " << name << " as " << texture.arrayId << ":" << texture.arraySlot << std::endl;

    // Store texture
    texturesByName[name] = texture;
    return &texturesByName[name];
}

unsigned char *TextureManager::generateMipMaps(
    uint32_t width, uint32_t height, uint32_t mipLevels, void *source, vk::DeviceSize *outSize
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

    *outSize = imageSize;
    return combinedPixels;
}

vk::DescriptorSet TextureManager::getBinding(uint32_t arrayId, uint32_t samplerId, vk::Sampler sampler) {
    auto it = textureArrays.find(arrayId);
    assert(it != textureArrays.end());

    return it->second->getDescriptor(samplerId, sampler, device.device, descriptorPool, descriptorSetLayout);
}

vk::ImageView TextureManager::getTextureView(const Texture &texture) const {
    auto it = textureArrays.find(texture.arrayId);
    assert(it != textureArrays.end());

    return it->second->getImage()->imageView();
}

void TextureManager::generatePlaceholders() {
    VkDeviceSize imageSize = PLACEHOLDER_TEXTURE_SIZE * PLACEHOLDER_TEXTURE_SIZE;

    auto *pixels = new uint32_t[imageSize];
    generateErrorPixels(PLACEHOLDER_TEXTURE_SIZE, PLACEHOLDER_TEXTURE_SIZE, pixels);

    errorTexture = createTexture("internal.error")
        .fromRaw(PLACEHOLDER_TEXTURE_SIZE, PLACEHOLDER_TEXTURE_SIZE, pixels)
        .withMipMode(MipType::NoMipmap)
        .build();

    generateSolidPixels(PLACEHOLDER_TEXTURE_SIZE, PLACEHOLDER_TEXTURE_SIZE, pixels, 0x00000000);

    createTexture("internal.loading")
        .fromRaw(PLACEHOLDER_TEXTURE_SIZE, PLACEHOLDER_TEXTURE_SIZE, pixels)
        .withMipMode(MipType::NoMipmap)
        .build();

    generateSolidPixels(PLACEHOLDER_TEXTURE_SIZE, PLACEHOLDER_TEXTURE_SIZE, pixels, 0xFFFFFFFF);

    createTexture("internal.white")
        .fromRaw(PLACEHOLDER_TEXTURE_SIZE, PLACEHOLDER_TEXTURE_SIZE, pixels)
        .withMipMode(MipType::NoMipmap)
        .build();

    delete[] pixels;

}

}
