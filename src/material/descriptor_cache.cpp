#include <vulkanutils.hpp>
#include "descriptor_cache.hpp"
#include "tech-core/device.hpp"
#include "tech-core/texture/texture.hpp"
#include "tech-core/material/material.hpp"
#include "tech-core/image.hpp"
#include "../texture/sampler_cache.hpp"

namespace Engine::Internal {

MaterialDescriptorCache::MaterialDescriptorCache(VulkanDevice &device, BufferManager &bufferManager)
    : device(device),
    bufferManager(bufferManager) {

    std::array<vk::DescriptorPoolSize, 2> sizes = {
        vk::DescriptorPoolSize {
            vk::DescriptorType::eCombinedImageSampler,
            // TODO: This will probably be an issue some time.
            9999
        },
        vk::DescriptorPoolSize {
            vk::DescriptorType::eUniformBuffer,
            // TODO: This will probably be an issue some time.
            9999
        }
    };

    pool = device.device.createDescriptorPool(
        {
            {},
            9999,
            vkUseArray(sizes)
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

    auto uniformBuffers = createUniformBuffers(material);
    auto layout = createLayout(material);
    auto set = createSet(material, layout, uniformBuffers);

    descriptors[material] = {
        layout,
        set,
        std::move(uniformBuffers)
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

std::unordered_map<uint32_t, std::unique_ptr<Buffer>> MaterialDescriptorCache::createUniformBuffers(
    const Material *material
) const {
    std::unordered_map<uint32_t, std::unique_ptr<Buffer>> buffers;

    auto variables = material->getVariables();
    for (auto &var : variables) {
        if (var.type == ShaderBindingType::Uniform) {
            auto value = material->getUniformUntyped(var.name);
            assert(!value.empty());

            // TODO: Move this to be a GPU Only buffer, and transfer the data across using staging buffer.
            auto buffer = bufferManager.aquire(
                var.uniformSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryUsage::eCPUToGPU
            );

            buffer->copyIn(value.getRaw(), value.getSize());
            buffer->flush();

            buffers.emplace(var.bindingId, std::move(buffer));
        }
    }

    return buffers;
}

vk::DescriptorSet MaterialDescriptorCache::createSet(
    const Material *material, vk::DescriptorSetLayout layout,
    const std::unordered_map<uint32_t, std::unique_ptr<Buffer>> &buffers
) const {
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
            case ShaderBindingType::Uniform: {
                auto value = material->getUniformUntyped(var.name);
                assert(!value.empty());

                auto buffer = buffers.at(var.bindingId)->buffer();

                vk::DescriptorBufferInfo bufferInfo {
                    buffer,
                    0,
                    value.getSize()
                };

                vk::WriteDescriptorSet update {
                    sets[0],
                    var.bindingId,
                    0,
                    1,
                    vk::DescriptorType::eUniformBuffer,
                    nullptr,
                    &bufferInfo
                };

                device.device.updateDescriptorSets(1, &update, 0, nullptr);
                break;
            }
            default:
                assert(false);
                break;
        }
    }

    return sets[0];
}

}