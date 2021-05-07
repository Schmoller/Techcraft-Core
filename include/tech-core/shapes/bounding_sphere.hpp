#pragma once
#include <glm/glm.hpp>

namespace Engine {

class BoundingBox;

class BoundingSphere {
    public:
    float x;
    float y;
    float z;
    float radius;

    BoundingSphere() = default;
    BoundingSphere(const glm::vec3 &center, float radius);
    BoundingSphere(float x, float y, float z, float radius);

    bool operator==(const BoundingSphere &other) const;
    BoundingSphere operator+(const glm::vec3 &position) const;
    BoundingSphere operator-(const glm::vec3 &position) const;
    BoundingSphere &operator+=(const glm::vec3 &position);
    BoundingSphere &operator-=(const glm::vec3 &position);

    BoundingSphere include(const glm::vec3 &position) const;
    BoundingSphere &includeSelf(const glm::vec3 &position);

    bool contains(const BoundingSphere &other) const;
    bool contains(const glm::vec3 &point) const;

    bool intersects(const BoundingSphere &other) const;
    bool intersects(const BoundingBox &other) const;


    glm::vec3 center() const;

};

}