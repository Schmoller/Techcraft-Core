#pragma once

#include "tech-core/forward.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Engine {

class Transform final {
public:
    explicit Transform(Entity &);

    const glm::vec3 &getPosition() const { return position; };

    const glm::quat &getRotation() const { return rotation; };

    const glm::vec3 &getScale() const { return scale; };

    const glm::mat4 &getTransform() const { return transform; }

    void setPosition(const glm::vec3 &);
    void setRotation(const glm::quat &);
    void setScale(const glm::vec3 &);
    void setScale(float);
    void setTransform(const glm::mat4 &);

private:
    Entity &owner;

    glm::vec3 position {};
    glm::quat rotation {};
    glm::vec3 scale { 1, 1, 1 };
    // Combined transformation matrix
    glm::mat4 transform { 1 };

    void updateTransform();
};

}