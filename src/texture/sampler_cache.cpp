#include "sampler_cache.hpp"
#include "tech-core/device.hpp"

namespace Engine::Internal {
SamplerCache::SamplerCache(VulkanDevice &device)
    : device(device) {

}

vk::SamplerAddressMode convertToAddressMode(TextureWrapMode mode) {
    switch (mode) {
        default:
        case TextureWrapMode::Repeat:
            return vk::SamplerAddressMode::eRepeat;
        case TextureWrapMode::Mirror:
            return vk::SamplerAddressMode::eMirroredRepeat;
        case TextureWrapMode::Clamp:
            return vk::SamplerAddressMode::eClampToEdge;
        case TextureWrapMode::Border:
            return vk::SamplerAddressMode::eClampToBorder;
    }
}

std::shared_ptr<SamplerRef> SamplerCache::acquire(const SamplerSettings &settings) {
    auto it = samplers.find(settings);
    if (it != samplers.end()) {
        auto sampler = it->second.lock();

        if (sampler) {
            return sampler;
        }
    }

    vk::Filter filter;
    switch (settings.filtering) {
        case TextureFilterMode::None:
            filter = vk::Filter::eNearest;
            break;
        case TextureFilterMode::Linear:
            filter = vk::Filter::eLinear;
            break;
        case TextureFilterMode::Cubic:
            // TODO: This needs to be enabled and check that its supported etc.
//            filter = vk::Filter::eCubicIMG;
            filter = vk::Filter::eLinear;
            break;
    }

    vk::SamplerAddressMode addressingU = convertToAddressMode(settings.wrapU);
    vk::SamplerAddressMode addressingV = convertToAddressMode(settings.wrapV);
    vk::SamplerMipmapMode mipMapping;
    float maxLod;
    if (settings.mipMaps) {
        mipMapping = vk::SamplerMipmapMode::eLinear;
        maxLod = 16;
    } else {
        mipMapping = vk::SamplerMipmapMode::eNearest;
        maxLod = 0;
    }

    auto sampler = device.device.createSampler(
        {
            {},
            filter, filter,
            mipMapping,
            addressingU,
            addressingV,
            vk::SamplerAddressMode::eRepeat,
            0,
            settings.anisotropy > 0,
            settings.anisotropy,
            false,
            {},
            0,
            maxLod
        }
    );

    auto samplerRef = std::make_shared<SamplerRef>(device.device, sampler);
    samplers[settings] = samplerRef;

    return samplerRef;
}

SamplerRef::SamplerRef(vk::Device device, vk::Sampler sampler)
    : device(device), sampler(sampler) {
}

SamplerRef::~SamplerRef() {
    device.destroySampler(sampler);
}

bool SamplerSettings::operator==(const SamplerSettings &other) const {
    return (
        mipMaps == other.mipMaps &&
            anisotropy == other.anisotropy &&
            filtering == other.filtering &&
            wrapU == other.wrapU &&
            wrapV == other.wrapV
    );
}
}