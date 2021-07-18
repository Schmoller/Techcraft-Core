#include <vulkanutils.hpp>
#include "descriptor_cache.hpp"
#include "tech-core/device.hpp"
#include "tech-core/texture/texture.hpp"
#include "tech-core/material/material.hpp"
#include "tech-core/image.hpp"
#include "../texture/sampler_cache.hpp"

namespace Engine::Internal {

MaterialDescriptorCache::MaterialDescriptorCache(VulkanDevice &device)
    : device(device) {

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

MaterialDescriptorCache::~MaterialDescriptorCache() {
    for (auto &descriptor : descriptors) {
        device.device.destroy(descriptor.second.layout);
    }
    device.device.destroy(pool);
}

vk::DescriptorSet MaterialDescriptorCache::get(const Material *material) {
    auto it = descriptors.find(material);
    if (it != descriptors.end()) {
        return it->second.set;
    }

    auto layout = createLayout(material);
    auto set = createSet(material, layout);

    descriptors[material] = {
        layout,
        set
    };

    return set;
}

vk::DescriptorSetLayout MaterialDescriptorCache::createLayout(const Material *material) const {
    auto variables = material->getVariables();
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    bindings.reserve(variables.size());

    for (auto &var : variables) {
        vk::ShaderStageFlags stage;
        switch (var.stage) {
            case ShaderStageType::Vertex:
                stage = vk::ShaderStageFlagBits::eVertex;
                break;
            case ShaderStageType::Fragment:
                stage = vk::ShaderStageFlagBits::eFragment;
                break;
            default:
                assert(false);
                break;
        }

        switch (var.type) {
            case ShaderBindingType::Texture:
                bindings.emplace_back(
                    var.bindingId,
                    vk::DescriptorType::eCombinedImageSampler,
                    1,
                    stage
                );
                break;
            case ShaderBindingType::Uniform:
                bindings.emplace_back(
                    var.bindingId,
                    vk::DescriptorType::eUniformBuffer,
                    1,
                    stage
                );
                break;
        }
    }

    return device.device.createDescriptorSetLayout(
        {
            {},
            vkUseArray(bindings)
        }
    );
}

vk::DescriptorSet MaterialDescriptorCache::createSet(const Material *material, vk::DescriptorSetLayout layout) const {
    auto sets = device.device.allocateDescriptorSets(
        {
            pool,
            1, &layout
        }
    );

    auto variables = material->getVariables();
    for (auto &var : variables) {
        switch (var.type) {
            case ShaderBindingType::Texture: {
                auto texture = material->getTexture(var.name);
                auto sampler = texture->getSampler()->get();

                vk::DescriptorImageInfo imageInfo {
                    sampler,
                    texture->getImage()->imageView(),
                    vk::ImageLayout::eShaderReadOnlyOptimal
                };

                vk::WriteDescriptorSet update {
                    sets[0],
                    var.bindingId,
                    0,
                    1,
                    vk::DescriptorType::eCombinedImageSampler,
                    &imageInfo
                };

                device.device.updateDescriptorSets(1, &update, 0, nullptr);
                break;
            }
            case ShaderBindingType::Uniform:
                // TODO: Setup buffers and transfer stuff etc. Or do we do that somewhere else?
                assert(false);
                break;
            default:
                assert(false);
                break;
        }
    }

    return sets[0];
}

}