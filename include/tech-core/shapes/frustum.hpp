#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>

namespace Engine {

// Forward
class Plane;

class Frustum {
public:

    void update(const glm::mat4 &viewProj);

    inline bool intersects(const glm::vec3 &minPoint, const glm::vec3 &maxPoint) const {
        for (int i = 0; i < 6; i++) {
            if (planes[i].x * minPoint.x + planes[i].y * maxPoint.y + planes[i].z * minPoint.z + planes[i].w >
                0)
                continue;
            if (planes[i].x * minPoint.x + planes[i].y * maxPoint.y + planes[i].z * maxPoint.z + planes[i].w >
                0)
                continue;
            if (planes[i].x * maxPoint.x + planes[i].y * maxPoint.y + planes[i].z * maxPoint.z + planes[i].w >
                0)
                continue;
            if (planes[i].x * maxPoint.x + planes[i].y * maxPoint.y + planes[i].z * minPoint.z + planes[i].w >
                0)
                continue;
            if (planes[i].x * maxPoint.x + planes[i].y * minPoint.y + planes[i].z * minPoint.z + planes[i].w >
                0)
                continue;
            if (planes[i].x * maxPoint.x + planes[i].y * minPoint.y + planes[i].z * maxPoint.z + planes[i].w >
                0)
                continue;
            if (planes[i].x * minPoint.x + planes[i].y * minPoint.y + planes[i].z * maxPoint.z + planes[i].w >
                0)
                continue;
            if (planes[i].x * minPoint.x + planes[i].y * minPoint.y + planes[i].z * minPoint.z + planes[i].w >
                0)
                continue;
            return false;
        }
        return true;
    }

    Plane planeLeft() const;
    Plane planeRight() const;
    Plane planeTop() const;
    Plane planeBottom() const;
    Plane planeFar() const;
    Plane planeNear() const;

private:
    glm::vec4 planes[6];
};

}