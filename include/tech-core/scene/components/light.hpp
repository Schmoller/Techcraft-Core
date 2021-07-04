#pragma once

#include "base.hpp"
#include <glm/glm.hpp>

namespace Engine {

enum class LightType {
    Directional,
    Point,
    Spot
};

class Light final : public Component {
public:
    explicit Light(Entity &);

    void setType(LightType);
    void setRange(float);
    void setIntensity(float);
    void setColor(const glm::vec3 &);

    LightType getType() const { return type; }

    float getRange() const { return range; }

    float getIntensity() const { return intensity; }

    const glm::vec3 &getColor() const { return color; }

private:
    LightType type { LightType::Directional };
    float range { 10 };
    float intensity { 1 };
    glm::vec3 color { 1, 1, 1 };
};

}