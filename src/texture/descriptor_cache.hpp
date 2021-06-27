#pragma once

#include "tech-core/forward.hpp"
#include <vulkan/vulkan.hpp>

namespace Engine::Internal {

class DescriptorCache {
public:
    explicit DescriptorCache(VulkanDevice &device, uint32_t binding);
    ~DescriptorCache();

    vk::DescriptorSet get(const Texture *);
private:
    VulkanDevice &device;
    uint32_t binding;

    vk::DescriptorSetLayout layout;
    vk::DescriptorPool pool;
    std::unordered_map<const Texture *, vk::DescriptorSet> descriptors;
};

class DescriptorCacheManager {
public:
    explicit DescriptorCacheManager(VulkanDevice &device);
    std::shared_ptr<DescriptorCache> get(uint32_t binding);

private:
    VulkanDevice &device;
    std::unordered_map<uint32_t, std::weak_ptr<DescriptorCache>> caches;
};

}



