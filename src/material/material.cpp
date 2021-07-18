#include "tech-core/material/material.hpp"
#include "tech-core/material/builder.hpp"
#include "tech-core/shader/shader.hpp"

namespace Engine {

Material::Material(const MaterialBuilder &builder)
    : name(builder.getName()),
    textures(builder.textures),
    shader(builder.shader) {

    assert(builder.shader);

    // Put in empty slots for variables that should exist.
    // MaterialManager will fill these slots in with transparent textures afterwards
    for (auto &var : shader->getVariables()) {
        if (var.type == ShaderBindingType::Texture) {
            if (!textures.contains(var.name)) {
                textures[var.name] = nullptr;
            }
        }
    }
}

void Material::setShader(std::shared_ptr<Shader> newShader) {
    shader = std::move(newShader);
}

std::vector<ShaderVariable> Material::getVariables() const {
    return shader->getVariables(ShaderBindingUsage::Material);
}

bool Material::hasVariable(const std::string_view &variable) const {
    auto vars = getVariables();

    for (auto &var : vars) {
        if (var.name == variable) {
            return true;
        }
    }

    return false;
}

const Texture *Material::getTexture(const std::string &variable) const {
    assert(textures.contains(variable));
    return textures.at(variable);
}

void Material::setTexture(const std::string &variable, const Texture *texture) {
    assert(textures.contains(variable));
    textures[variable] = texture;
}

}