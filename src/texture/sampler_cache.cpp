#include "sampler_cache.hpp"

namespace Engine::Internal {
SamplerCache::SamplerCache(VulkanDevice &device)
    : device(device) {

}

std::shared_ptr<SamplerRef> SamplerCache::acquire(const SamplerSettings &settings) {
    return std::shared_ptr<SamplerRef>();
}

SamplerRef::SamplerRef(vk::Device device, vk::Sampler sampler)
    : device(device), sampler(sampler) {
}

SamplerRef::~SamplerRef() {
    device.destroySampler(sampler);
}
}