#pragma once

#include "tech-core/forward.hpp"
#include <deque>
#include <memory>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

#define TEXTURE_BINDING 2
#define DESCRIPTOR_POOL_SIZE 9999

namespace Engine::Internal {
// FIXME: Eventually I want these texture arrays to be automatically calculated after all textures are loaded
/*
 * It would be better for these to be automatically computed to be the most efficient size possible.
 * Currently we just have fixed size arrays and if we don't use all the elements in it we're just wasting memory.
 */
class TextureArray {
public:
    TextureArray(uint32_t id, std::shared_ptr<Image> image, uint32_t mipLevels);

    uint32_t getId() const { return id; }

    std::shared_ptr<Image> getImage() const { return image; }

    uint32_t getMipLevels() const { return mipLevels; }

    bool canAllocate(uint32_t width, uint32_t height) const;
    uint32_t allocateSlot();

    /*
     * TODO: Eventually moved descriptor set creation to its own system which can look at the big picture and produce the set of descriptor sits required
     * to make everything
     */
    vk::DescriptorSet getDescriptor(
        uint32_t samplerId, vk::Sampler sampler, vk::Device device, vk::DescriptorPool pool,
        vk::DescriptorSetLayout layout
    );

private:
    uint32_t id;

    // Free slots in the array for new images
    std::deque<size_t> freeSlots;
    // The image array itself
    uint32_t mipLevels;
    std::shared_ptr<Image> image;
    std::unordered_map<uint32_t, vk::DescriptorSet> descriptorBySampler;
};

}