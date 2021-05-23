#include "tech-core/shapes/bounding_box.hpp"
#include "tech-core/shapes/bounding_sphere.hpp"
#include "tech-core/shapes/frustum.hpp"

namespace Engine {

// BoundingBox
BoundingBox::BoundingBox(const glm::vec3 &a, const glm::vec3 &b) {
    xMin = std::min(a.x, b.x);
    yMin = std::min(a.y, b.y);
    zMin = std::min(a.z, b.z);
    xMax = std::max(a.x, b.x);
    yMax = std::max(a.y, b.y);
    zMax = std::max(a.z, b.z);
}

BoundingBox::BoundingBox(float xMin, float yMin, float zMin, float xMax, float yMax, float zMax)
    : xMin(xMin), yMin(yMin), zMin(zMin), xMax(xMax), yMax(yMax), zMax(zMax) {}

BoundingBox::BoundingBox(float width, float depth, float height, bool centered) {
    if (centered) {
        xMin = -width / 2;
        yMin = -depth / 2;
        zMin = -height / 2;
        xMax = width / 2;
        yMax = depth / 2;
        zMax = height / 2;
    } else {
        xMin = 0;
        yMin = 0;
        zMin = 0;
        xMax = width;
        yMax = depth;
        zMax = height;
    }
}

bool BoundingBox::operator==(const BoundingBox &other) const {
    return (
        xMin == other.xMin &&
            yMin == other.yMin &&
            zMin == other.zMin &&
            xMax == other.xMax &&
            yMax == other.yMax &&
            zMax == other.zMax
    );
}

BoundingBox BoundingBox::operator+(const glm::vec3 &position) const {
    return {
        xMin + position.x,
        yMin + position.y,
        zMin + position.z,
        xMax + position.x,
        yMax + position.y,
        zMax + position.z
    };
}

BoundingBox BoundingBox::operator-(const glm::vec3 &position) const {
    return {
        xMin - position.x,
        yMin - position.y,
        zMin - position.z,
        xMax - position.x,
        yMax - position.y,
        zMax - position.z
    };
}

BoundingBox &BoundingBox::operator+=(const glm::vec3 &position) {
    xMin += position.x;
    yMin += position.y;
    zMin += position.z;
    xMax += position.x;
    yMax += position.y;
    zMax += position.z;

    return *this;
}

BoundingBox &BoundingBox::operator-=(const glm::vec3 &position) {
    xMin -= position.x;
    yMin -= position.y;
    zMin -= position.z;
    xMax -= position.x;
    yMax -= position.y;
    zMax -= position.z;

    return *this;
}

BoundingBox &BoundingBox::offsetX(float x) {
    xMin += x;
    xMax += x;
    return *this;
}

BoundingBox &BoundingBox::offsetY(float y) {
    yMin += y;
    yMax += y;
    return *this;
}

BoundingBox &BoundingBox::offsetZ(float z) {
    zMin += z;
    zMax += z;
    return *this;
}

BoundingBox BoundingBox::include(const glm::vec3 &position) const {
    return {
        std::min(xMin, static_cast<float>(position.x)),
        std::min(yMin, static_cast<float>(position.y)),
        std::min(zMin, static_cast<float>(position.z)),
        std::max(xMax, static_cast<float>(position.x)),
        std::max(yMax, static_cast<float>(position.y)),
        std::max(zMax, static_cast<float>(position.z))
    };
}

BoundingBox &BoundingBox::includeSelf(const glm::vec3 &position) {
    xMin = std::min(xMin, static_cast<float>(position.x));
    yMin = std::min(yMin, static_cast<float>(position.y));
    zMin = std::min(zMin, static_cast<float>(position.z));
    xMax = std::max(xMax, static_cast<float>(position.x));
    yMax = std::max(yMax, static_cast<float>(position.y));
    zMax = std::max(zMax, static_cast<float>(position.z));
    return *this;
}

BoundingBox BoundingBox::expand(float x, float y, float z) const {
    return {
        xMin - x,
        yMin - y,
        zMin - z,
        xMax + x,
        yMax + y,
        zMax + z
    };
}

BoundingBox BoundingBox::expand(float all) const {
    return expand(all, all, all);
}

BoundingBox BoundingBox::expandSkew(float x, float y, float z) const {
    return {
        (x < 0 ? xMin + x : xMin),
        (y < 0 ? yMin + y : yMin),
        (z < 0 ? zMin + z : zMin),
        (x > 0 ? xMax + x : xMax),
        (y > 0 ? yMax + y : yMax),
        (z > 0 ? zMax + z : zMax)
    };
}

BoundingBox &BoundingBox::expandSkewSelf(float x, float y, float z) {
    if (x > 0) {
        xMax += x;
    } else {
        xMin += x;
    }
    if (y > 0) {
        yMax += y;
    } else {
        yMin += y;
    }
    if (z > 0) {
        zMax += z;
    } else {
        zMin += z;
    }

    return *this;
}

BoundingBox BoundingBox::shrinkSkew(float x, float y, float z) const {
    return {
        (x > 0 ? xMin + x : xMin),
        (y > 0 ? yMin + y : yMin),
        (z > 0 ? zMin + z : zMin),
        (x < 0 ? xMax + x : xMax),
        (y < 0 ? yMax + y : yMax),
        (z < 0 ? zMax + z : zMax)
    };
}

BoundingBox &BoundingBox::shrinkSkewSelf(float x, float y, float z) {
    if (x < 0) {
        xMax += x;
    } else {
        xMin += x;
    }
    if (y < 0) {
        yMax += y;
    } else {
        yMin += y;
    }
    if (z < 0) {
        zMax += z;
    } else {
        zMin += z;
    }

    return *this;
}

bool BoundingBox::contains(const BoundingBox &other) const {
    return (
        xMin <= other.xMin && other.xMax <= xMax &&
            yMin <= other.yMin && other.yMax <= yMax &&
            zMin <= other.zMin && other.zMax <= zMax
    );
}

bool BoundingBox::contains(const glm::vec3 &point) const {
    return (
        xMin <= point.x && point.x <= xMax &&
            yMin <= point.y && point.y <= yMax &&
            zMin <= point.z && point.z <= zMax
    );
}

bool BoundingBox::intersects(const BoundingBox &other) const {
    return (
        xMin < other.xMax && xMax > other.xMin &&
            yMin < other.yMax && yMax > other.yMin &&
            zMin < other.zMax && zMax > other.zMin
    );
}

float square(float val) {
    return val * val;
}

bool BoundingBox::intersects(const BoundingSphere &other) const {
    auto squareRadius = other.radius * other.radius;
    auto minimum = 0;
    if (other.x < xMin) {
        minimum += square(other.x - xMin);
    } else if (other.x > xMax) {
        minimum += square(other.x - xMax);
    }

    if (other.y < yMin) {
        minimum += square(other.y - yMin);
    } else if (other.y > yMax) {
        minimum += square(other.y - yMax);
    }

    if (other.z < zMin) {
        minimum += square(other.z - zMin);
    } else if (other.z > zMax) {
        minimum += square(other.z - zMax);
    }

    return (minimum <= squareRadius);
}

bool BoundingBox::intersects(const Frustum &other) const {
    return other.intersects(
        { xMin, yMin, zMin },
        { xMax, yMax, zMax }
    );
}

bool BoundingBox::intersectedBy(
    const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection, HitAxis &outHitFace, float *outDistance
) {
    glm::vec3 fractionalDir = 1.0f / static_cast<glm::vec3>(rayDirection);

    float tXMin = (xMin - rayOrigin.x) * fractionalDir.x;
    float tXMax = (xMax - rayOrigin.x) * fractionalDir.x;
    float tYMin = (yMin - rayOrigin.y) * fractionalDir.y;
    float tYMax = (yMax - rayOrigin.y) * fractionalDir.y;
    float tZMin = (zMin - rayOrigin.z) * fractionalDir.z;
    float tZMax = (zMax - rayOrigin.z) * fractionalDir.z;

    float tX, tY, tZ;
    tX = std::min(tXMin, tXMax);
    tY = std::min(tYMin, tYMax);
    tZ = std::min(tZMin, tZMax);

    float tClosePlane = std::max(std::max(tX, tY), tZ);
    float tFarPlane = std::min(std::min(std::max(tXMin, tXMax), std::max(tYMin, tYMax)), std::max(tZMin, tZMax));

    // Hit exists behind origin
    if (tFarPlane < 0) {
        return false;
    }

    // Hit not present within the AABB
    if (tClosePlane > tFarPlane) {
        return false;
    }

    // Determine hit dir
    if (tX == tClosePlane) {
        if (rayDirection.x > 0) {
            outHitFace = HitAxis::PosX;
        } else {
            outHitFace = HitAxis::NegX;
        }
    } else if (tY == tClosePlane) {
        if (rayDirection.y > 0) {
            outHitFace = HitAxis::PosY;
        } else {
            outHitFace = HitAxis::NegY;
        }
    } else if (tZ == tClosePlane) {
        if (rayDirection.z > 0) {
            outHitFace = HitAxis::PosZ;
        } else {
            outHitFace = HitAxis::NegZ;
        }
    }

    if (outDistance) {
        *outDistance = tClosePlane;
    }

    return true;
}

glm::vec3 BoundingBox::center() const {
    return {
        (xMin + xMax) / 2.0f,
        (yMin + yMax) / 2.0f,
        (zMin + zMax) / 2.0f
    };
}

float BoundingBox::width() const {
    return (xMax - xMin);
}

float BoundingBox::depth() const {
    return (yMax - yMin);
}

float BoundingBox::height() const {
    return (zMax - zMin);
}

bool BoundingBox::intersectsRay(const glm::vec3 &origin, const glm::vec3 &direction) const {
    double minIntersect = -std::numeric_limits<double>::infinity();
    double maxIntersect = std::numeric_limits<double>::infinity();

    if (direction.x != 0.0) {
        double minXIntersect = (xMin - origin.x) / direction.x;
        double maxXIntersect = (xMax - origin.x) / direction.x;

        minIntersect = std::max(minIntersect, std::min(minXIntersect, maxXIntersect));
        maxIntersect = std::min(maxIntersect, std::max(minXIntersect, maxXIntersect));
    }

    if (direction.y != 0.0) {
        double minYIntersect = (yMin - origin.y) / direction.y;
        double maxYIntersect = (yMax - origin.y) / direction.y;

        minIntersect = std::max(minIntersect, std::min(minYIntersect, maxYIntersect));
        maxIntersect = std::min(maxIntersect, std::max(minYIntersect, maxYIntersect));
    }

    if (direction.z != 0.0) {
        double minZIntersect = (zMin - origin.z) / direction.z;
        double maxZIntersect = (zMax - origin.z) / direction.z;

        minIntersect = std::max(minIntersect, std::min(minZIntersect, maxZIntersect));
        maxIntersect = std::min(maxIntersect, std::max(minZIntersect, maxZIntersect));
    }

    return maxIntersect >= minIntersect;
}

bool BoundingBox::intersectsRay(
    const glm::vec3 &origin, const glm::vec3 &direction, glm::vec3 &enter, glm::vec3 &exit
) const {
    double minIntersect = -std::numeric_limits<double>::infinity();
    double maxIntersect = std::numeric_limits<double>::infinity();

    if (direction.x != 0.0) {
        double minXIntersect = (xMin - origin.x) / direction.x;
        double maxXIntersect = (xMax - origin.x) / direction.x;

        minIntersect = std::max(minIntersect, std::min(minXIntersect, maxXIntersect));
        maxIntersect = std::min(maxIntersect, std::max(minXIntersect, maxXIntersect));
    }

    if (direction.y != 0.0) {
        double minYIntersect = (yMin - origin.y) / direction.y;
        double maxYIntersect = (yMax - origin.y) / direction.y;

        minIntersect = std::max(minIntersect, std::min(minYIntersect, maxYIntersect));
        maxIntersect = std::min(maxIntersect, std::max(minYIntersect, maxYIntersect));
    }

    if (direction.z != 0.0) {
        double minZIntersect = (zMin - origin.z) / direction.z;
        double maxZIntersect = (zMax - origin.z) / direction.z;

        minIntersect = std::max(minIntersect, std::min(minZIntersect, maxZIntersect));
        maxIntersect = std::min(maxIntersect, std::max(minZIntersect, maxZIntersect));
    }

    if (maxIntersect >= minIntersect) {
        enter = origin + direction * static_cast<float>(std::max(minIntersect, 0.0));
        exit = origin + direction * static_cast<float>(std::max(maxIntersect, 0.0));

        return true;
    } else {
        return false;
    }
}


}