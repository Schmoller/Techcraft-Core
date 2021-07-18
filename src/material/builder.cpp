#include "tech-core/material/builder.hpp"
#include "tech-core/material/manager.hpp"
#include "tech-core/texture/common.hpp"

namespace Engine {


MaterialBuilder::MaterialBuilder(std::string name, MaterialManager &manager)
    : name(std::move(name)), manager(manager) {
}

MaterialBuilder &MaterialBuilder::withShader(std::shared_ptr<Shader> newShader) {
    shader = std::move(newShader);
    return *this;
}

Material *MaterialBuilder::build() const {
    assert(shader);
    return manager.add(*this);
}

MaterialBuilder &MaterialBuilder::withTexture(const std::string &variable, const Texture *texture) {
    textures[variable] = texture;
    return *this;
}

MaterialBuilder &MaterialBuilder::withTexture(const std::string_view &variable, const Texture *texture) {
    textures[std::string(variable)] = texture;
    return *this;
}

}