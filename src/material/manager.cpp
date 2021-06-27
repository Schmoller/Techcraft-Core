#include "tech-core/material/manager.hpp"
#include "tech-core/material/material.hpp"
#include "tech-core/material/builder.hpp"
#include "tech-core/texture/manager.hpp"

namespace Engine {

Material *MaterialManager::get(const std::string &name) {
    auto it = materials.find(name);
    if (it != materials.end()) {
        return it->second.get();
    }

    return nullptr;
}

const Material *MaterialManager::get(const std::string &name) const {
    auto it = materials.find(name);
    if (it != materials.end()) {
        return it->second.get();
    }

    return nullptr;
}

MaterialBuilder MaterialManager::add(const std::string &name) {
    return MaterialBuilder(name, *this);
}

Material *MaterialManager::add(const MaterialBuilder &builder) {
    assert(materials.count(builder.getName()) == 0);

    auto material = std::make_shared<Material>(builder);
    if (!material->getAlbedo()) {
        material->setAlbedo(textureManager.getWhite());
    }
    if (!material->getNormal()) {
        material->setNormal(textureManager.getTransparent());
    }

    materials[builder.getName()] = material;

    return material.get();
}

void MaterialManager::remove(const Material *material) {
    auto it = materials.find(material->getName());
    if (it != materials.end()) {
        materials.erase(it);
    }
}

void MaterialManager::remove(const std::string &name) {
    auto it = materials.find(name);
    if (it != materials.end()) {
        materials.erase(it);
    }
}

std::vector<const Material *> MaterialManager::getMaterials() const {
    std::vector<const Material *> materialVec(materials.size());
    size_t index = 0;
    for (auto &pair : materials) {
        materialVec[index] = pair.second.get();
        ++index;
    }
    return materialVec;
}

MaterialManager::MaterialManager(TextureManager &textureManager)
    : textureManager(textureManager) {
    generateDefaultMaterials();
}

void MaterialManager::generateDefaultMaterials() {
    defaultMaterial = add("internal.default")
        .build();
}


}