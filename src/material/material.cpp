#include "tech-core/material/material.hpp"
#include "tech-core/material/builder.hpp"

namespace Engine {

Material::Material(const MaterialBuilder &builder)
    : name(builder.getName()),
    albedo(builder.albedo),
    albedoColor(builder.albedoColor),
    normal(builder.normal),
    textureScale(builder.textureScale),
    textureOffset(builder.textureOffset),
    shader(builder.shader) {

}

void Material::setAlbedo(const Texture *texture) {
    albedo = texture;
}

void Material::setAlbedoColor(const glm::vec4 &color) {
    albedoColor = color;
}

void Material::setNormal(const Texture *texture) {
    normal = texture;
}

void Material::setTextureScale(const glm::vec2 &scale) {
    textureScale = scale;
}

void Material::setTextureOffset(const glm::vec2 &offset) {
    textureOffset = offset;
}

void Material::setShader(std::shared_ptr<Shader> newShader) {
    shader = std::move(newShader);
}
}