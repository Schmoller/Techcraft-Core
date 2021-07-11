#include "tech-core/material/builder.hpp"
#include "tech-core/material/manager.hpp"
#include "tech-core/texture/common.hpp"

namespace Engine {


MaterialBuilder::MaterialBuilder(std::string name, MaterialManager &manager)
    : name(std::move(name)), manager(manager) {
}

MaterialBuilder &MaterialBuilder::withAlbedo(const Texture *texture) {
    albedo = texture;
    return *this;
}

MaterialBuilder &MaterialBuilder::withAlbedoColor(const glm::vec4 &color) {
    albedoColor = color;
    return *this;
}

MaterialBuilder &MaterialBuilder::withNormal(const Texture *texture) {
    normal = texture;
    return *this;
}

MaterialBuilder &MaterialBuilder::withTextureScale(const glm::vec2 &scale) {
    textureScale = scale;
    return *this;
}

MaterialBuilder &MaterialBuilder::withTextureOffset(const glm::vec2 &offset) {
    textureOffset = offset;
    return *this;
}

MaterialBuilder &MaterialBuilder::withShader(std::shared_ptr<Shader> newShader) {
    shader = std::move(newShader);
    return *this;
}

Material *MaterialBuilder::build() const {
    return manager.add(*this);
}

}