#pragma once

#include "tech-core/forward.hpp"
#include <vulkan/vulkan.hpp>

namespace Engine::Internal {

class MaterialDescriptorCache {
public:
    explicit MaterialDescriptorCache(VulkanDevice &device);
    ~MaterialDescriptorCache();

    vk::DescriptorSet get(const Material *);
private:
    struct MaterialDescriptors {
        vk::DescriptorSetLayout layout;
        vk::DescriptorSet set;
    };
    VulkanDevice &device;

    vk::DescriptorPool pool;
    std::unordered_map<const Material *, MaterialDescriptors> descriptors;

    vk::DescriptorSetLayout createLayout(const Material *) const;
    vk::DescriptorSet createSet(const Material *, vk::DescriptorSetLayout) const;
};

}



