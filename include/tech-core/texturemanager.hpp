#ifndef TEXTUREMANAGER_HPP
#define TEXTUREMANAGER_HPP

#include "forward.hpp"
#include "common_includes.hpp"
#include "image.hpp"
#include "tech-core/buffer.hpp"
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>

/*
* Requirements:
* Must place textures into slots in a texture array
* Must manage these texture arrays
* Must generate the descriptor sets required to render with the texture
*/

namespace Engine {

struct TextureArray {
    TextureArray(
        uint32_t arrayId = 0,
        uint32_t width = 0,
        uint32_t height = 0,
        uint32_t mipLevels = 0
    ) :
        arrayId(arrayId),
        width(width),
        height(height),
        mipLevels(mipLevels) {
    }

    uint32_t arrayId;

    // Size of the images contained within the array
    uint32_t width;
    uint32_t height;

    // Free slots in the array for new images
    std::deque<size_t> freeSlots;
    // The image array itself
    uint32_t mipLevels;
    vk::Image textureArray;
    vk::ImageView textureView;
    vk::ImageView textureNonMipView;
    VmaAllocation textureMemory;
};

struct Texture {
    uint32_t arrayId;
    size_t arraySlot;
    bool mipmaps;
    uint32_t width;
    uint32_t height;
};

struct DescriptorPair {
    vk::DescriptorSet mipmap;
    vk::DescriptorSet nonMipmap;
};

class TextureBuilder {
    friend class TextureManager;
public:
    /**
     * Sources the pixel data from a texture on the filesystem.
     */
    TextureBuilder &fromFile(const std::string &filename);
    /**
     * Sources the pixel data from raw data.
     * The pixel format is expected to be RGBA 8 bits per pixel.
     * The pixels array must have width * height elements.
     */
    TextureBuilder &fromRaw(uint32_t width, uint32_t height, uint32_t *pixels);
    /**
     * Changes the mipmap mode.
     */
    TextureBuilder &withMipMode(MipType type);
    Texture *build();

private:
    TextureBuilder(TextureManager &manager, const std::string &name);

    // Non-configurable
    TextureManager &manager;
    std::string name;

    // Configurable
    void *pixelData;
    bool sourcedFromFile;
    uint32_t width;
    uint32_t height;
    MipType mipType;
};

class TextureManager {
    friend class TextureBuilder;

public:
    void initialize(
        vk::Device device,
        VmaAllocator allocator,
        vk::CommandPool commandPool,
        vk::Queue submitQueue,
        vk::DescriptorSetLayout descriptorLayout
    );

    void destroy();

    TextureBuilder createTexture(const std::string &name);
    Texture *getTexture(const std::string &name);

    Texture *getTexture(const char *name) {
        return getTexture(std::string(name));
    }

    void destroyTexture(const std::string &name);

    void destroyTexture(const char *name) {
        destroyTexture(std::string(name));
    }

    void onSwapChainRecreate();

    inline vk::DescriptorSet getBinding(const Texture &texture, uint32_t samplerId, const vk::Sampler &sampler) {
        return getBinding(texture.arrayId, samplerId, sampler, texture.mipmaps);
    }

    vk::DescriptorSet getBinding(uint32_t arrayId, uint32_t samplerId, const vk::Sampler &sampler, bool mipmaps = true);

    vk::DescriptorSetLayout getLayout() {
        return descriptorLayout;
    }

private:
    std::unordered_map<uint32_t, TextureArray> textureArrays;
    uint32_t nextArrayId = 0;
    std::unordered_map<std::string, Texture> textures;
    std::unordered_map<uint64_t, DescriptorPair> descriptors;

    // Required vulkan stuff
    vk::Device device;
    VmaAllocator allocator;
    vk::CommandPool commandPool;
    vk::Queue submitQueue;

    // Descriptors
    vk::DescriptorPool descriptorPool;
    vk::DescriptorSetLayout descriptorLayout;

    // Transfer Temp
    vk::CommandBuffer oneTimeCommands;
    Buffer stagingBuffer;
    TextureArray *activeArray;
    uint32_t activeSlot;

    Texture *createTexture(const std::string &name, uint32_t width, uint32_t height, void *pixelData, MipType mipType);

    TextureArray &selectTextureArray(uint32_t width, uint32_t height, bool requireSlot);
    void beginTransfer(void *pixels, vk::DeviceSize size, TextureArray &array, size_t slot);
    void
    transferIntoSlot(uint32_t offsetX, uint32_t offsetY, const TextureArray &array, size_t slot, uint32_t mipLevel);
    void transferIntoSlotAlt(const TextureArray &array, size_t slot, uint32_t mipLevel, vk::DeviceSize offset);
    void endTransfer();

    DescriptorPair createDescriptorSet(TextureArray &array, uint32_t samplerId, const vk::Sampler &sampler);
};

class TextureLoadError : public std::exception {
public:
    TextureLoadError(const std::string &filename)
        : filename(filename) {}

    virtual const char *what() const noexcept override {
        return filename.c_str();
    }

private:
    std::string filename;
};

}

#endif