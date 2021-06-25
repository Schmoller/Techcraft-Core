#include "tech-core/material/builder.hpp"
#include "tech-core/material/manager.hpp"
#include "tech-core/texture/common.hpp"

namespace Engine {


Engine::MaterialBuilder::MaterialBuilder(std::string name, MaterialManager &manager)
    : name(std::move(name)), manager(manager) {
}

MaterialBuilder &Engine::MaterialBuilder::withAlbedo(const Texture *texture) {
    albedo = texture;
    return *this;
}

MaterialBuilder &Engine::MaterialBuilder::withAlbedoColor(const glm::vec4 &color) {
    albedoColor = color;
    return *this;
}

MaterialBuilder &Engine::MaterialBuilder::withNormal(const Texture *texture) {
    normal = texture;
    return *this;
}

MaterialBuilder &Engine::MaterialBuilder::withTextureScale(const glm::vec2 &scale) {
    textureScale = scale;
    return *this;
}

MaterialBuilder &Engine::MaterialBuilder::withTextureOffset(const glm::vec2 &offset) {
    textureOffset = offset;
    return *this;
}

Material *Engine::MaterialBuilder::build() const {
    return manager.add(*this);
}

}