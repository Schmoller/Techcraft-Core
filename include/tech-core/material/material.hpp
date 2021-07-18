#pragma once

#include "common.hpp"
#include "names.hpp"
#include "tech-core/forward.hpp"
#include "tech-core/shader/common.hpp"
#include "tech-core/utilities/any.hpp"
#include <glm/glm.hpp>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

namespace Engine {


class Material {
public:
    explicit Material(const MaterialBuilder &builder);

    const std::string &getName() const { return name; }

    const std::shared_ptr<Shader> &getShader() const { return shader; };

    std::vector<ShaderVariable> getVariables() const;
    bool hasVariable(const std::string_view &variable) const;

    const Texture *getTexture(const std::string &variable) const;
    void setTexture(const std::string &variable, const Texture *);

    template<typename T>
    const T &getUniform(const ShaderUniformId<T> &id) const;
    Any getUniformUntyped(const std::string &variable) const;
    template<typename T>
    void setUniform(const ShaderUniformId<T> &id, const T &value);

    void setShader(std::shared_ptr<Shader>);

private:
    const std::string name;

    std::shared_ptr<Shader> shader;

    std::unordered_map<std::string, const Texture *> textures;
    std::unordered_map<std::string, Any> uniforms;

    void setUniformUntyped(const std::string &variable, Any value);
};

template<typename T>
const T &Material::getUniform(const ShaderUniformId<T> &id) const {
    auto value = getUniformUntyped(std::string(id.name));
    assert(!value.empty());

    return value.get<T>();
}

template<typename T>
void Material::setUniform(const ShaderUniformId<T> &id, const T &value) {
    setUniformUntyped(std::string(id.name), value);
}

}
