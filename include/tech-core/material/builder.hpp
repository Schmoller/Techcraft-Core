#pragma once

#include "tech-core/forward.hpp"
#include <glm/glm.hpp>
#include <string>

namespace Engine {

class MaterialBuilder {
    friend class Material;
public:
    MaterialBuilder(std::string name, MaterialManager &manager);

    const std::string &getName() const { return name; }

    MaterialBuilder &withAlbedo(const Texture *);
    MaterialBuilder &withAlbedoColor(const glm::vec4 &);
    MaterialBuilder &withNormal(const Texture *);
    MaterialBuilder &withTextureScale(const glm::vec2 &);
    MaterialBuilder &withTextureOffset(const glm::vec2 &);

    Material *build() const;
private:
    // Provided
    const std::string name;
    MaterialManager &manager;

    const Texture *albedo { nullptr };
    glm::vec4 albedoColor { 1, 1, 1, 1 };

    const Texture *normal { nullptr };

    glm::vec2 textureScale { 1, 1 };
    glm::vec2 textureOffset {};
};

}
