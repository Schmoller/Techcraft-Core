#pragma once

#include "../forward.hpp"
#include "common.hpp"
#include "texture.hpp"
#include <string>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

namespace Engine {

class TextureManager {
    friend class TextureBuilder;
public:
    TextureManager(RenderEngine &engine, VulkanDevice &device, vk::PhysicalDevice);

    TextureBuilder add(const std::string &name);
    const Texture *add(const TextureBuilder &);

    const Texture *get(const std::string &name) const;
    bool remove(const std::string &name);

private:
    RenderEngine &engine;
    VulkanDevice &device;
    std::shared_ptr<Internal::SamplerCache> samplers;

    bool canBlitTextures { false };
    float maxAnisotropy { 0 };

    std::unordered_map<std::string, SharedTexture> texturesByName;
    const Texture *errorTexture { nullptr };

    void generatePlaceholders();

    static void generateMipmaps(vk::CommandBuffer buffer, const std::shared_ptr<Image> &image);
    static unsigned char *generateMipMapsFallback(
        uint32_t width, uint32_t height, uint32_t mipLevels, void *source, vk::DeviceSize *outputSize
    );
};

}



