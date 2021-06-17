#pragma once

#include "../forward.hpp"
#include "common.hpp"
#include <string>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

namespace Engine {

class TextureManager {
    friend class TextureBuilder;
public:
    TextureManager(RenderEngine &engine, VulkanDevice &device);
    ~TextureManager();

    TextureBuilder createTexture(const std::string &name);
    const Texture *getTexture(const std::string &name) const;
    bool removeTexture(const std::string &name);

    vk::ImageView getTextureView(const Texture &texture) const;
    vk::DescriptorSet getBinding(uint32_t arrayId, uint32_t samplerId, vk::Sampler sampler);

    inline vk::DescriptorSet getBinding(const Texture &texture, uint32_t samplerId, vk::Sampler sampler) {
        return getBinding(texture.arrayId, samplerId, sampler);
    }

    vk::DescriptorSetLayout getLayout() {
        return descriptorSetLayout;
    }

private: // For TextureBuilder
    const Texture *addTexture(
        const std::string &name, uint32_t width, uint32_t height, void *pixelData, MipType mipType
    );

private:
    RenderEngine &engine;
    VulkanDevice &device;

    // FIXME: I dont think this is safe with us passing texture pointers around
    std::unordered_map<std::string, Texture> texturesByName;

    std::unordered_map<uint32_t, std::shared_ptr<Internal::TextureArray>> textureArrays;
    uint32_t nextArrayId { 0 };

    vk::DescriptorPool descriptorPool;
    vk::DescriptorSetLayout descriptorSetLayout;

    const Texture *errorTexture { nullptr };

    Internal::TextureArray &allocateArray(uint32_t width, uint32_t height);
    static unsigned char *generateMipMaps(
        uint32_t width, uint32_t height, uint32_t mipLevels, void *source, vk::DeviceSize *outputSize
    );

    void generatePlaceholders();
};

}



