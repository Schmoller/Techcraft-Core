#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include "common_includes.hpp"
#include "texturemanager.hpp"

#include <string>
#include <unordered_map>
#include <functional>

namespace Engine {

struct MaterialCreateInfo {
    MaterialCreateInfo(
        const std::string &name,
        const std::string &diffuseTexture,
        const std::string &normalTexture,
        const std::string &shader,
        bool useMipmaps = true,
        vk::Filter minFilter = vk::Filter::eLinear,
        vk::Filter magFilter = vk::Filter::eLinear,
        vk::SamplerMipmapMode mipmapMode = vk::SamplerMipmapMode::eLinear
    ) : name(name),
        diffuseTexture(diffuseTexture),
        normalTexture(normalTexture),
        shader(shader),
        minFilter(minFilter),
        magFilter(magFilter),
        mipmapMode(mipmapMode),
        useMipmaps(useMipmaps)
    {}

    const std::string &name;
    const std::string diffuseTexture;
    const std::string normalTexture;
    const std::string &shader;

    // Texture sampler settings
    const vk::Filter minFilter;
    const vk::Filter magFilter;
    const vk::SamplerMipmapMode mipmapMode;
    const bool useMipmaps;
};

struct Material {
    uint32_t samplerId;
    Texture *diffuseTexture;
    Texture *normalTexture;
};

struct SamplerDefinition {
    const vk::Filter minFilter;
    const vk::Filter magFilter;
    const vk::SamplerMipmapMode mipmapMode;
    const bool useMipmaps;

    bool operator==(const SamplerDefinition &other) const {
        return minFilter == other.minFilter &&
            magFilter == other.magFilter &&
            mipmapMode == other.mipmapMode &&
            useMipmaps == other.useMipmaps;
    }
};

}

namespace std {
    template<> struct hash<Engine::SamplerDefinition> {
        std::size_t operator()(Engine::SamplerDefinition const& sampler) const noexcept {
            auto samplerHash = std::hash<vk::Filter>()(sampler.minFilter);
            samplerHash = samplerHash ^ (std::hash<vk::Filter>()(sampler.magFilter) << 1);
            samplerHash = samplerHash ^ (std::hash<vk::SamplerMipmapMode>()(sampler.mipmapMode) << 1);
            samplerHash = samplerHash ^ (std::hash<bool>()(sampler.useMipmaps) << 1);

            return samplerHash;
        }
    };
}

namespace Engine {

class MaterialManager {
    public:
    void initialize(vk::Device device, TextureManager *textureManager);
    void destroy();
    
    Material *addMaterial(const MaterialCreateInfo &materialInfo);
    Material *getMaterial(const std::string &name);
    void destroyMaterial(const std::string &name);

    vk::DescriptorSet getBinding(const Material &material);
    vk::Sampler getSampler(const Material &material);
    vk::Sampler getSamplerById(uint32_t id);
    uint32_t createSampler(const SamplerDefinition &definition);

    private:
    vk::Device device;
    TextureManager *textureManager;

    std::unordered_map<std::string, Material> materials;
    std::unordered_map<SamplerDefinition, uint32_t> samplerDefinitions;
    uint32_t nextSamplerId = 0;
    std::unordered_map<uint32_t, vk::Sampler> samplers;

};

}

#endif