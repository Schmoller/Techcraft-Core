#pragma once

#include "names.hpp"
#include "tech-core/forward.hpp"
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

    MaterialBuilder &withShader(std::shared_ptr<Shader>);

    Material *build() const;
private:
    // Provided
    const std::string name;
    MaterialManager &manager;

    std::shared_ptr<Shader> shader;
    std::unordered_map<std::string, const Texture *> textures;
};

}
