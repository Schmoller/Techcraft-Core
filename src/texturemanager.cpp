#include "texturemanager.hpp"
#include "stb_image.h"
#include <iostream>

namespace Engine {

// Memory for 5 4k 32bit textures
const VkDeviceSize MAX_MEMORY_USAGE = 335544320;

const uint32_t MAX_ARRAY_SIZE = 16;

#define TEXTURE_BINDING 2
#define DESCRIPTOR_POOL_SIZE 9999

TextureBuilder::TextureBuilder(TextureManager &manager, const std::string &name)
  : manager(manager),
    name(name),
    pixelData(nullptr),
    width(0),
    height(0),
    mipType(MipType::NoMipmap),
    sourcedFromFile(false)
{}

TextureBuilder &TextureBuilder::fromFile(const std::string &filename) {
    int texWidth, texHeight, texChannels;

    stbi_uc *pixels = stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        throw TextureLoadError(filename);
    }

    if (this->pixelData && this->sourcedFromFile) {
        stbi_image_free(this->pixelData);
    }

    this->pixelData = pixels;
    this->width = texWidth;
    this->height = texHeight;
    this->sourcedFromFile = true;

    return *this;
}

TextureBuilder &TextureBuilder::fromRaw(uint32_t width, uint32_t height, uint32_t *pixels) {
    if (this->pixelData && this->sourcedFromFile) {
        stbi_image_free(this->pixelData);
    }

    this->pixelData = pixels;
    this->width = width;
    this->height = height;
    this->sourcedFromFile = false;

    return *this;
}

TextureBuilder &TextureBuilder::withMipMode(MipType type) {
    mipType = type;

    return *this;
}

Texture *TextureBuilder::build() {
    if (!pixelData || width == 0 || height == 0) {
        throw std::runtime_error("Incomplete texture definition");
    }

    Texture *texture = manager.createTexture(name, width, height, pixelData, mipType);

    if (sourcedFromFile) {
        stbi_image_free(pixelData);
    }

    return texture;
}


void TextureManager::initialize(
    vk::Device device,
    VmaAllocator allocator,
    vk::CommandPool commandPool,
    vk::Queue submitQueue,
    vk::DescriptorSetLayout descriptorLayout) {
    
    this->device = device;
    this->allocator = allocator;
    this->commandPool = commandPool;
    this->submitQueue = submitQueue;
    this->descriptorLayout = descriptorLayout;

    // Allocate a descriptor pool
    vk::DescriptorPoolSize poolSize(
        vk::DescriptorType::eCombinedImageSampler,
        DESCRIPTOR_POOL_SIZE
    );

    descriptorPool = device.createDescriptorPool({{}, DESCRIPTOR_POOL_SIZE, 1, &poolSize});
}

void TextureManager::onSwapChainRecreate() {
    device.destroyDescriptorPool(descriptorPool);

    // Allocate a descriptor pool
    vk::DescriptorPoolSize poolSize(
        vk::DescriptorType::eCombinedImageSampler,
        DESCRIPTOR_POOL_SIZE
    );

    descriptorPool = device.createDescriptorPool({{}, DESCRIPTOR_POOL_SIZE, 1, &poolSize});

    // Release all existing descriptor sets
    descriptors.clear();
}

void TextureManager::destroy() {
    device.destroyDescriptorPool(descriptorPool);

    for (auto &pair : textureArrays) {
        auto &array = pair.second;
        device.destroyImageView(array.textureView);
        device.destroyImageView(array.textureNonMipView);
        vmaDestroyImage(allocator, array.textureArray, array.textureMemory);
    }
}

Texture *TextureManager::createTexture(const std::string &name, uint32_t width, uint32_t height, void *pixels, MipType mipType) {
    if (mipType == MipType::StoredStandard) {
        // Standard mipmaps are stored on the right of the main texture using 50% more width
        width *= 2.0/3.0;
    }

    vk::DeviceSize imageSize = width * height * 4;

    // Find a place to store the texture
    TextureArray &array = selectTextureArray(width, height, true);
    size_t slot = array.freeSlots.front();
    array.freeSlots.pop_front();

    Texture texture = {
        array.arrayId,
        slot,
        mipType != MipType::NoMipmap,
        width,
        height
    };

    // Queue up the transfer into the array
    beginTransfer(pixels, imageSize, array, slot);

    if (mipType == MipType::NoMipmap) {
        transferIntoSlot(0, 0, array, slot, 0);
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

        for (uint32_t level = 0; level < array.mipLevels; ++level) {
            if (level == 0) {
                // Base Level
                transferIntoSlot(0, 0, array, slot, 0);
            } else {
                // Mip Levels
                if (mipHeight > 1) {
                    mipHeight >>= 1;
                }

                transferIntoSlot(width, offsetY, array, slot, level);
                offsetY += mipHeight;
            }
        }
    } else {
        transferIntoSlot(0, 0, array, slot, 0);
        generateMipmaps(array, slot, width, height);
    }

    endTransfer();

    std::cout << "Loaded texture " << name << " as " << texture.arrayId << ":" << texture.arraySlot << std::endl;

    // Store texture
    textures[name] = texture;
    return &textures[name];
}

TextureBuilder TextureManager::createTexture(const std::string &name) {
    return TextureBuilder(*this, name);
}

Texture *TextureManager::getTexture(const std::string &name) {
    if (textures.count(name) > 0) {
        return &textures[name];
    }

    return nullptr;
}

void TextureManager::destroyTexture(const std::string &name) {
    Texture *texture = getTexture(name);

    if (!texture) {
        return;
    }

    textures.erase(name);

    // Release the slot back to the array
    TextureArray &array = textureArrays[texture->arrayId];
    array.freeSlots.push_back(texture->arraySlot);

    delete texture;

    // TODO: Allow for array reclamation
}


TextureArray &TextureManager::selectTextureArray(uint32_t width, uint32_t height, bool requireSlot) {
    for (auto &arrays : textureArrays) {
        auto &array = arrays.second;
        
        if (array.width == width && array.height == height) {
            if (!requireSlot || array.freeSlots.size() > 0) {
                return array;
            }
        }
    }

    // No free slots in any texure arrays, or not matching sizes
    std::cout << "Creating texture array for size " << width << "x" << height << std::endl;

    uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

    TextureArray array(nextArrayId++, width, height, mipLevels);

    // Allocate the array
    VkDeviceSize imageUsage = width * height * 4;
    uint32_t arraySize = std::min(static_cast<uint32_t>(MAX_MEMORY_USAGE / imageUsage), MAX_ARRAY_SIZE);
    
    vk::ImageCreateInfo createInfo(
        {},
        vk::ImageType::e2D,
        vk::Format::eR8G8B8A8Unorm,
        {width, height, 1},
        mipLevels,
        arraySize,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::SharingMode::eExclusive
    );

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VkImageCreateInfo createInfoOld = static_cast<VkImageCreateInfo>(createInfo);
    VkImage imageTemp;

    VkResult result = vmaCreateImage(
        allocator,
        &createInfoOld,
        &allocInfo,
        &imageTemp,
        &array.textureMemory,
        nullptr
    );
    
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create texture pool");
    }

    array.textureArray = vk::Image(imageTemp);

    // Allocate views of the array
    vk::ImageViewCreateInfo mipViewInfo(
        {},
        array.textureArray,
        vk::ImageViewType::e2DArray,
        vk::Format::eR8G8B8A8Unorm,
        {},
        { vk::ImageAspectFlagBits::eColor, 0, mipLevels, 0, arraySize }
    );

    array.textureView = device.createImageView(mipViewInfo);

    vk::ImageViewCreateInfo nonMipViewInfo(
        {},
        array.textureArray,
        vk::ImageViewType::e2DArray,
        vk::Format::eR8G8B8A8Unorm,
        {},
        { vk::ImageAspectFlagBits::eColor, 0, 1, 0, arraySize } // Select only the root texture, no mips
    );

    array.textureNonMipView = device.createImageView(nonMipViewInfo);

    // Fill free slots
    for (size_t i = 0; i < arraySize; ++i) {
        array.freeSlots.push_back(i);
    }

    // Initialize the image
    vk::CommandBufferAllocateInfo commandAllocInfo(
        commandPool,
        vk::CommandBufferLevel::ePrimary,
        1
    );
    
    oneTimeCommands = device.allocateCommandBuffers(commandAllocInfo)[0];
    oneTimeCommands.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    // Switch the entire array to shader read ready
    vk::ImageMemoryBarrier barrier(
        {}, vk::AccessFlagBits::eShaderRead,
        vk::ImageLayout::eUndefined, vk::ImageLayout::eShaderReadOnlyOptimal,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        array.textureArray,
        { vk::ImageAspectFlagBits::eColor, 0, array.mipLevels, 0, arraySize }
    );

    oneTimeCommands.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eFragmentShader,
        {},
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    oneTimeCommands.end();

    vk::SubmitInfo submitInfo(
        0, nullptr,
        nullptr,
        1, &oneTimeCommands
    );
    
    submitQueue.submit(1, &submitInfo, vk::Fence());
    submitQueue.waitIdle();

    textureArrays[array.arrayId] = array;
    return textureArrays[array.arrayId];
}

void TextureManager::beginTransfer(void *pixels, vk::DeviceSize size, TextureArray &array, size_t slot) {
    stagingBuffer.allocate(
        allocator, size, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryUsage::eCPUOnly
    );
    stagingBuffer.copyIn(pixels, size);

    // Prepare command buffer for execution
    vk::CommandBufferAllocateInfo allocInfo(
        commandPool,
        vk::CommandBufferLevel::ePrimary,
        1
    );
    
    oneTimeCommands = device.allocateCommandBuffers(allocInfo)[0];
    oneTimeCommands.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    // Prepare the array for writing to the slot
    vk::ImageMemoryBarrier barrier(
        vk::AccessFlagBits::eShaderRead, vk::AccessFlagBits::eTransferWrite,
        vk::ImageLayout::eShaderReadOnlyOptimal, vk::ImageLayout::eTransferDstOptimal,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        array.textureArray,
        { vk::ImageAspectFlagBits::eColor, 0, array.mipLevels, static_cast<uint32_t>(slot), 1 }
    );

    oneTimeCommands.pipelineBarrier(
        vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eTransfer,
        {},
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    activeArray = &array;
    activeSlot = slot;
}

void TextureManager::transferIntoSlot(uint32_t offsetX, uint32_t offsetY, const TextureArray &array, size_t slot, uint32_t mipLevel) {
    uint32_t factor = mipLevel > 0 ? (1 << (mipLevel-1)) : 0; 
    uint32_t width = array.width >> factor;
    uint32_t height = array.height >> factor;

    if (width < 1) {
        width = 1;
    }

    if (height < 1) {
        height = 1;
    }

    // Copy the buffer over
    vk::BufferImageCopy region(
        0, 0, 0,
        { vk::ImageAspectFlagBits::eColor, mipLevel, static_cast<uint32_t>(slot), 1},
        { static_cast<int32_t>(offsetX), static_cast<int32_t>(offsetY), 0 },
        { width, height, 1 }
    );

    oneTimeCommands.copyBufferToImage(stagingBuffer.buffer(), array.textureArray, vk::ImageLayout::eTransferDstOptimal, 1, &region);
}

void TextureManager::generateMipmaps(const TextureArray &array, size_t slot, uint32_t texWidth, uint32_t texHeight, uint32_t startLevel) {
    // TODO: Generate MIPS
    throw std::runtime_error("TODO: Generate MIPS");
}

void TextureManager::endTransfer() {
    // Transition slot back to be shader readable
    vk::ImageMemoryBarrier barrier(
         vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead,
        vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        activeArray->textureArray,
        { vk::ImageAspectFlagBits::eColor, 0, activeArray->mipLevels, activeSlot, 1 }
    );

    oneTimeCommands.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
        {},
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    // Execute the transfer
    oneTimeCommands.end();

    vk::SubmitInfo submitInfo(
        0, nullptr,
        nullptr,
        1, &oneTimeCommands
    );
    
    submitQueue.submit(1, &submitInfo, vk::Fence());
    submitQueue.waitIdle();

    // Cleanup
    device.freeCommandBuffers(commandPool, 1, &oneTimeCommands);
    stagingBuffer.destroy();
    activeArray = nullptr;
}

DescriptorPair TextureManager::createDescriptorSet(TextureArray &array, uint32_t samplerId, const vk::Sampler &sampler) {
    std::array<vk::DescriptorSetLayout, 2> descriptorLayouts = {descriptorLayout, descriptorLayout};
    vk::DescriptorSetAllocateInfo descriptorAllocInfo(
        descriptorPool,
        2,
        descriptorLayouts.data()
    );

    auto descriptorSets = device.allocateDescriptorSets(descriptorAllocInfo);
    DescriptorPair descriptorPair = {
        descriptorSets[0],
        descriptorSets[1]
    };

    vk::DescriptorImageInfo imageInfoMipmap(
        sampler,
        array.textureView,
        vk::ImageLayout::eShaderReadOnlyOptimal
    );

    vk::DescriptorImageInfo imageInfoNoMipmap(
        sampler,
        array.textureNonMipView,
        vk::ImageLayout::eShaderReadOnlyOptimal
    );

    std::array<vk::WriteDescriptorSet, 2> descriptorWrites = {
        vk::WriteDescriptorSet(
            descriptorPair.mipmap,
            TEXTURE_BINDING,
            0,
            1,
            vk::DescriptorType::eCombinedImageSampler,
            &imageInfoMipmap
        ),
        vk::WriteDescriptorSet(
            descriptorPair.nonMipmap,
            TEXTURE_BINDING,
            0,
            1,
            vk::DescriptorType::eCombinedImageSampler,
            &imageInfoNoMipmap
        )
    };

    device.updateDescriptorSets(descriptorWrites, {});

    uint64_t descriptorSetId = ((uint64_t)array.arrayId) << 32 | samplerId;
    descriptors[descriptorSetId] = descriptorPair;
    return descriptorPair;
}

vk::DescriptorSet TextureManager::getBinding(uint32_t arrayId, uint32_t samplerId, const vk::Sampler &sampler, bool mipmaps) {
    uint64_t descriptorSetId = ((uint64_t)arrayId) << 32 | samplerId;

    DescriptorPair pair;
    if (!descriptors.count(descriptorSetId)) {
        pair = createDescriptorSet(textureArrays[arrayId], samplerId, sampler);
    } else {
        pair = descriptors[descriptorSetId];
    }

    if (mipmaps) {
        return pair.mipmap;
    } else {
        return pair.nonMipmap;
    }
}

}