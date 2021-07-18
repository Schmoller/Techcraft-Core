#include "tech-core/material/material.hpp"

#include <utility>
#include "tech-core/material/builder.hpp"
#include "tech-core/shader/shader.hpp"

namespace Engine {

Material::Material(const MaterialBuilder &builder)
    : name(builder.getName()),
    textures(builder.textures),
    uniforms(builder.uniforms),
    shader(builder.shader) {

    assert(builder.shader);

    // Put in empty slots for variables that should exist.
    // MaterialManager will fill these slots in with transparent textures afterwards
    for (auto &var : shader->getVariables()) {
        if (var.type == ShaderBindingType::Texture) {
            if (!textures.contains(var.name)) {
                textures[var.name] = nullptr;
            }
        } else if (var.type == ShaderBindingType::Uniform) {
            if (!uniforms.contains(var.name)) {
                uniforms.emplace(var.name, Any {});
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

Any Material::getUniformUntyped(const std::string &variable) const {
    assert(uniforms.contains(variable));
    return uniforms.at(variable);
}

void Material::setUniformUntyped(
    const std::string &variable, Any value
) {
    assert(uniforms.contains(variable));
    uniforms.emplace(
        variable, std::move(value)
    );
}

}