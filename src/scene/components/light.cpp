#include "tech-core/scene/components/light.hpp"
#include "tech-core/scene/entity.hpp"

namespace Engine {


Light::Light(Entity &entity) : Component(entity) {

}

void Light::setType(LightType lightType) {
    type = lightType;
    owner.invalidate(EntityInvalidateType::Light);
}

void Light::setRange(float newRange) {
    range = std::max(newRange, 0.0f);
    owner.invalidate(EntityInvalidateType::Light);
}

void Light::setIntensity(float newIntensity) {
    intensity = std::max(newIntensity, 0.0f);
    owner.invalidate(EntityInvalidateType::Light);
}

void Light::setColor(const glm::vec3 &newColor) {
    color = newColor;
    owner.invalidate(EntityInvalidateType::Light);
}

}