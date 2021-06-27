#include "descriptor_cache.hpp"
#include "tech-core/device.hpp"
#include "tech-core/texture/texture.hpp"
#include "tech-core/image.hpp"
#include "sampler_cache.hpp"

namespace Engine::Internal {

DescriptorCache::DescriptorCache(VulkanDevice &device, uint32_t binding)
    : device(device), binding(binding) {

    vk::DescriptorSetLayoutBinding bindingDescription {
        binding,
        vk::DescriptorType::eCombinedImageSampler,
        1,
        vk::ShaderStageFlagBits::eFragment,
    };

    layout = device.device.createDescriptorSetLayout(
        {
            {},
            1, &bindingDescription
        }
    );

    vk::DescriptorPoolSize poolSize {
        vk::DescriptorType::eCombinedImageSampler,
        // TODO: This will probably be an issue some time.
        9999
    };

    pool = device.device.createDescriptorPool(
        {
            {},
            9999,
            1, &poolSize
        }
    );
}

DescriptorCache::~DescriptorCache() {
    device.device.destroy(pool);
    device.device.destroy(layout);
}

vk::DescriptorSet DescriptorCache::get(const Texture *texture) {
    auto it = descriptors.find(texture);
    if (it != descriptors.end()) {
        return it->second;
    }

    auto sets = device.device.allocateDescriptorSets(
        {
            pool,
            1, &layout
        }
    );

    descriptors[texture] = sets[0];

    auto sampler = texture->getSampler()->get();

    vk::DescriptorImageInfo imageInfo {
        sampler,
        texture->getImage()->imageView(),
        vk::ImageLayout::eShaderReadOnlyOptimal
    };

    vk::WriteDescriptorSet update {
        sets[0],
        binding,
        0,
        1,
        vk::DescriptorType::eCombinedImageSampler,
        &imageInfo
    };

    device.device.updateDescriptorSets(1, &update, 0, nullptr);

    return sets[0];
}

std::shared_ptr<DescriptorCache> DescriptorCacheManager::get(uint32_t binding) {
    auto it = caches.find(binding);
    if (it != caches.end()) {
        auto cache = it->second.lock();
        if (cache) {
            return cache;
        }
    }

    auto cache = std::make_shared<DescriptorCache>(device, binding);
    caches[binding] = cache;
    return cache;
}

DescriptorCacheManager::DescriptorCacheManager(VulkanDevice &device)
    : device(device) {

}

}