#pragma once

#include <glm/glm.hpp>

namespace Engine {

class BoundingSphere;
class Frustum;

enum HitAxis {
    PosX,
    NegX,
    PosY,
    NegY,
    PosZ,
    NegZ
};

class BoundingBox {
public:
    float xMin;
    float yMin;
    float zMin;
    float xMax;
    float yMax;
    float zMax;

    BoundingBox() = default;
    BoundingBox(const glm::vec3 &a, const glm::vec3 &b);
    BoundingBox(float xMin, float yMin, float zMin, float xMax, float yMax, float zMax);
    BoundingBox(float width, float depth, float height, bool centered = true);

    bool operator==(const BoundingBox &other) const;
    BoundingBox operator+(const glm::vec3 &position) const;
    BoundingBox operator-(const glm::vec3 &position) const;
    BoundingBox &operator+=(const glm::vec3 &position);
    BoundingBox &operator-=(const glm::vec3 &position);

    BoundingBox include(const glm::vec3 &position) const;
    BoundingBox include(const BoundingBox &) const;
    BoundingBox &includeSelf(const glm::vec3 &position);
    BoundingBox &includeSelf(const BoundingBox &);

    BoundingBox &offsetX(float x);
    BoundingBox &offsetY(float y);
    BoundingBox &offsetZ(float z);

    BoundingBox expand(float x, float y, float z) const;
    BoundingBox expand(float all) const;

    BoundingBox expandSkew(float x, float y, float z) const;
    BoundingBox &expandSkewSelf(float x, float y, float z);
    BoundingBox shrinkSkew(float x, float y, float z) const;
    BoundingBox &shrinkSkewSelf(float x, float y, float z);

    bool contains(const BoundingBox &other) const;
    bool contains(const glm::vec3 &point) const;

    bool intersects(const BoundingBox &other) const;
    bool intersects(const BoundingSphere &other) const;
    bool intersects(const Frustum &other) const;
    bool intersectsRay(const glm::vec3 &origin, const glm::vec3 &direction) const;
    bool intersectsRay(const glm::vec3 &origin, const glm::vec3 &direction, glm::vec3 &enter, glm::vec3 &exit) const;

    /**
     * Checks for an intersection of this bounding box by an infinitely long ray.
     * This doesnt need to check the end of the ray because the use case already
     * limits the test.
     */
    bool intersectedBy(
        const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection, HitAxis &outHitFace, float *outDistance = nullptr
    );

    glm::vec3 center() const;
    float width() const;
    float depth() const;
    float height() const;

};

}