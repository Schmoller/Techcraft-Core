#pragma once

#include "names.hpp"
#include "common.hpp"
#include "tech-core/forward.hpp"
#include "tech-core/utilities/any.hpp"
#include <glm/glm.hpp>
#include <string>
#include <memory>
#include <unordered_map>

namespace Engine {

class MaterialBuilder {
    friend class Material;
public:
    MaterialBuilder(std::string name, MaterialManager &manager);

    const std::string &getName() const { return name; }

    MaterialBuilder &withTexture(const std::string &variable, const Texture *);
    MaterialBuilder &withTexture(const std::string_view &variable, const Texture *);

    template<typename T>
    MaterialBuilder &withUniform(const ShaderUniformId<T> &id, const T &value);

    MaterialBuilder &withShader(std::shared_ptr<Shader>);

    Material *build() const;
private:
    // Provided
    const std::string name;
    MaterialManager &manager;

    std::shared_ptr<Shader> shader;
    std::unordered_map<std::string, const Texture *> textures;
    std::unordered_map<std::string, Any> uniforms;

    void setUniformUntyped(const std::string &variable, Any value);
};

template<typename T>
MaterialBuilder &MaterialBuilder::withUniform(const ShaderUniformId<T> &id, const T &value) {
    setUniformUntyped(std::string(id.name), value);
    return *this;
}

}
