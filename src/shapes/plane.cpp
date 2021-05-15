#include "tech-core/shapes/plane.hpp"
#include <glm/glm.hpp>
#include <limits>

namespace Engine {

Plane::Plane(const glm::vec4 &plane) : plane(plane) {
}

glm::vec3 Plane::normal() const {
    return glm::normalize(glm::vec3 { plane.x, plane.y, plane.z });
}


float Plane::offset() const {
    return plane.w;
}

glm::vec3 Plane::intersect(const Plane &other1, const Plane &other2) const {
    glm::vec3 m1 { plane.x, other1.plane.x, other2.plane.x };
    glm::vec3 m2 { plane.y, other1.plane.y, other2.plane.y };
    glm::vec3 m3 { plane.z, other1.plane.z, other2.plane.z };
    glm::vec3 d { -plane.w, -other1.plane.w, -other2.plane.w };

    auto u = glm::cross(m2, m3);
    auto v = glm::cross(m1, d);

    auto denormalization = glm::dot(m1, u);

    if (std::abs(denormalization) < std::numeric_limits<float>::epsilon()) {
        // No intersection
        return {};
    }

    return {
        glm::dot(d, u) / denormalization,
        glm::dot(m3, v) / denormalization,
        -glm::dot(m2, v) / denormalization
    };
}

}