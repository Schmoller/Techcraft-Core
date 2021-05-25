#include "tech-core/material.hpp"
#include "tech-core/texturemanager.hpp"

namespace Engine {

void MaterialManager::initialize(vk::Device device, TextureManager *textureManager) {
    this->device = device;
    this->textureManager = textureManager;
}

void MaterialManager::destroy() {
    for (auto &pair : samplers) {
        device.destroySampler(pair.second);
    }

    samplers.clear();
    materials.clear();
    samplerDefinitions.clear();
}

Material *MaterialManager::addMaterial(const MaterialCreateInfo &materialInfo) {
    // Determine texture sampler needs
    SamplerDefinition samplerDefinition = {
        materialInfo.minFilter,
        materialInfo.magFilter,
        materialInfo.mipmapMode,
        materialInfo.useMipmaps
    };

    uint32_t samplerId;
    if (samplerDefinitions.count(samplerDefinition)) {
        samplerId = samplerDefinitions[samplerDefinition];
    } else {
        samplerId = createSampler(samplerDefinition);
    }

    Texture *diffuse = nullptr, *normal = nullptr;
    if (materialInfo.diffuseTexture.size()) {
        diffuse = textureManager->getTexture(materialInfo.diffuseTexture);
    }
    if (materialInfo.normalTexture.size()) {
        normal = textureManager->getTexture(materialInfo.normalTexture);
    }

    Material mat = {
        samplerId,
        diffuse,
        normal
    };

    materials[materialInfo.name] = mat;
    return &materials[materialInfo.name];
}

Material *MaterialManager::getMaterial(const std::string &name) {
    if (materials.count(name)) {
        return &materials[name];
    }

    return nullptr;
}

void MaterialManager::destroyMaterial(const std::string &name) {
    // TODO: This

    // Destroy sampler if no longer required. Need to reference count the samplers
}

uint32_t MaterialManager::createSampler(const SamplerDefinition &definition) {
    vk::SamplerCreateInfo samplerInfo(
        {},
        definition.magFilter,
        definition.minFilter,
        definition.mipmapMode,
        vk::SamplerAddressMode::eRepeat,
        vk::SamplerAddressMode::eRepeat,
        vk::SamplerAddressMode::eRepeat,
        0.0f,
        VK_FALSE,// FIXME: Re-enable once we have proper transparency support. // VK_TRUE, // anisotropic filtering
        16
    );

    if (definition.useMipmaps) {
        // TODO: Not sure if this is dangerous or not
        samplerInfo.setMaxLod(16);
    }

    auto sampler = device.createSampler(samplerInfo);
    uint32_t id = nextSamplerId++;

    samplers[id] = sampler;

    return id;
}

vk::DescriptorSet MaterialManager::getBinding(const Material &material) {
    auto sampler = getSampler(material);

    if (material.diffuseTexture) {
        return textureManager->getBinding(*material.diffuseTexture, material.samplerId, sampler);
    } else {
        // TODO: We will need to handle different slots at some point
        return textureManager->getBinding(0, material.samplerId, sampler);
    }
}

vk::Sampler MaterialManager::getSampler(const Material &material) {
    return samplers[material.samplerId];
}

vk::Sampler MaterialManager::getSamplerById(uint32_t id) {
    return samplers[id];
}

}