#pragma once

#include "tech-core/texture/common.hpp"
#include "tech-core/forward.hpp"
#include <memory>
#include <vulkan/vulkan.hpp>
#include <unordered_map>
#include <functional>

namespace Engine::Internal {

struct SamplerSettings {
    TextureFilterMode filtering { TextureFilterMode::Linear };
    bool mipMaps { false };
    TextureWrapMode wrapU { TextureWrapMode::Repeat };
    TextureWrapMode wrapV { TextureWrapMode::Repeat };
    float anisotropy { 0 };

    bool operator== (const SamplerSettings &other) const;
};

}

namespace std {
template<>
struct hash<Engine::Internal::SamplerSettings> {
    size_t operator() (const Engine::Internal::SamplerSettings &settings) const {
        return ((hash<Engine::TextureFilterMode>()(settings.filtering) ^
            (hash<bool>()(settings.mipMaps) << 1)) >> 1) ^
            (hash<Engine::TextureWrapMode>()(settings.wrapU) << 1) ^
            (hash<Engine::TextureWrapMode>()(settings.wrapV) << 1) ^
            (hash<float>()(settings.anisotropy) << 1);
    }
};
}

namespace Engine::Internal {

class SamplerRef {
public:
    SamplerRef(vk::Device device, vk::Sampler sampler);
    ~SamplerRef();

    vk::Sampler get() const { return sampler; }
private:
    vk::Device device;
    vk::Sampler sampler;
};

class SamplerCache {
public:
    explicit SamplerCache(VulkanDevice &device);

    std::shared_ptr<SamplerRef> acquire(const SamplerSettings &settings);
private:
    VulkanDevice &device;
    std::unordered_map<SamplerSettings, std::weak_ptr<SamplerRef>> samplers;
};

}



