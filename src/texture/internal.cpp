#include "internal.hpp"

#include "tech-core/image.hpp"

Engine::Internal::TextureArray::TextureArray(uint32_t id, std::shared_ptr<Image> arrayImage, uint32_t mipLevels)
    : id(id), image(std::move(arrayImage)), mipLevels(mipLevels) {

    freeSlots.resize(image->getLayers());
    for (size_t i = 0; i < image->getLayers(); ++i) {
        freeSlots[i] = i;
    }
}

bool Engine::Internal::TextureArray::canAllocate(uint32_t width, uint32_t height) const {
    if (image->getWidth() == width && image->getHeight() == height) {
        if (!freeSlots.empty()) {
            return true;
        }
    }
    return false;
}

uint32_t Engine::Internal::TextureArray::allocateSlot() {
    auto slot = freeSlots.front();
    freeSlots.pop_front();
    return slot;
}

vk::DescriptorSet Engine::Internal::TextureArray::getDescriptor(
    uint32_t samplerId, vk::Sampler sampler, vk::Device device, vk::DescriptorPool pool, vk::DescriptorSetLayout layout
) {
    auto it = descriptorBySampler.find(samplerId);
    if (it != descriptorBySampler.end()) {
        return it->second;
    }

    vk::DescriptorSetAllocateInfo descriptorAllocInfo(
        pool,
        1, &layout
    );

    auto descriptorSet = device.allocateDescriptorSets(descriptorAllocInfo)[0];

    vk::DescriptorImageInfo imageInfoMipmap(
        sampler,
        image->imageView(),
        vk::ImageLayout::eShaderReadOnlyOptimal
    );

    vk::WriteDescriptorSet updateSet(
        descriptorSet,
        TEXTURE_BINDING,
        0,
        1,
        vk::DescriptorType::eCombinedImageSampler,
        &imageInfoMipmap
    );

    device.updateDescriptorSets(1, &updateSet, 0, nullptr);

    descriptorBySampler[samplerId] = descriptorSet;
    return descriptorSet;
}
