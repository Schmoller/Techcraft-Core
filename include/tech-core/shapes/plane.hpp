#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace Engine {
class Plane {
public:
    explicit Plane(const glm::vec4 &);

    glm::vec3 normal() const;
    float offset() const;

    glm::vec3 intersect(const Plane &other1, const Plane &other2) const;

    const glm::vec4 &getEquation() const { return plane; }

private:
    glm::vec4 plane;
};

}


