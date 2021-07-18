#pragma once

#include "names.hpp"
#include "tech-core/forward.hpp"
#include "tech-core/shader/common.hpp"
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
    void setShader(std::shared_ptr<Shader>);
private:
    const std::string name;

    std::shared_ptr<Shader> shader;

    std::unordered_map<std::string, const Texture *> textures;
};

}
