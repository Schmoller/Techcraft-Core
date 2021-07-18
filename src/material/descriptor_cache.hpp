#pragma once

#include "tech-core/forward.hpp"
#include <vulkan/vulkan.hpp>

namespace Engine::Internal {

class MaterialDescriptorCache {
public:
    MaterialDescriptorCache(VulkanDevice &device, BufferManager &);
    ~MaterialDescriptorCache();

    vk::DescriptorSet get(const Material *);
private:
    struct MaterialDescriptors {
        vk::DescriptorSetLayout layout;
        vk::DescriptorSet set;
        std::unordered_map<uint32_t, std::unique_ptr<Buffer>> uniforms;
    };
    VulkanDevice &device;
    BufferManager &bufferManager;

    vk::DescriptorPool pool;
    std::unordered_map<const Material *, MaterialDescriptors> descriptors;

    std::unordered_map<uint32_t, std::unique_ptr<Buffer>> createUniformBuffers(const Material *) const;
    vk::DescriptorSetLayout createLayout(const Material *) const;
    vk::DescriptorSet createSet(
        const Material *, vk::DescriptorSetLayout, const std::unordered_map<uint32_t, std::unique_ptr<Buffer>> &buffers
    ) const;
};

}



