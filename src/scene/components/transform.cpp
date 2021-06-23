#include "tech-core/scene/components/transform.hpp"
#include "tech-core/scene/entity.hpp"

namespace Engine {


Transform::Transform(Entity &entity)
    : owner(entity) {
    updateTransform();
}

void Transform::setPosition(const glm::vec3 &newPosition) {
    position = newPosition;
    updateTransform();
}

void Transform::setRotation(const glm::quat &newRotation) {
    rotation = newRotation;
    updateTransform();
}

void Transform::setScale(const glm::vec3 &newScale) {
    scale = newScale;
    updateTransform();
}

void Transform::setScale(float newScale) {
    scale = { newScale, newScale, newScale };
    updateTransform();
}

void Transform::setTransform(const glm::mat4 &newTransform) {
    transform = newTransform;
    owner.invalidate(EntityInvalidateType::Transform);
}

void Transform::updateTransform() {
    transform = glm::mat4(1.0f);
    transform = glm::translate(transform, position);
    transform *= glm::mat4_cast(rotation);
    transform = glm::scale(transform, scale);
    owner.invalidate(EntityInvalidateType::Transform);
}
}
