#pragma once

#include "tech-core/forward.hpp"
#include <glm/glm.hpp>
#include <string>
#include <memory>

namespace Engine {

class Material {
public:
    explicit Material(const MaterialBuilder &builder);

    const std::string &getName() const { return name; }

    const Texture *getAlbedo() const { return albedo; }

    const glm::vec4 &getAlbedoColor() const { return albedoColor; }

    const Texture *getNormal() const { return normal; }

    const glm::vec2 &getTextureScale() const { return textureScale; }

    const glm::vec2 &getTextureOffset() const { return textureOffset; }

    const std::shared_ptr<Shader> &getShader() const { return shader; };

    void setAlbedo(const Texture *);
    void setAlbedoColor(const glm::vec4 &);
    void setNormal(const Texture *);
    void setTextureScale(const glm::vec2 &);
    void setTextureOffset(const glm::vec2 &);
    void setShader(std::shared_ptr<Shader>);
private:
    const std::string name;

    std::shared_ptr<Shader> shader;

    const Texture *albedo { nullptr };
    glm::vec4 albedoColor { 1, 1, 1, 1 };

    const Texture *normal { nullptr };

    glm::vec2 textureScale { 1, 1 };
    glm::vec2 textureOffset {};
};

}
