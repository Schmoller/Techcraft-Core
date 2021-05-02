#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>

namespace Engine {

class Frustum {
    public:

    void update(const glm::mat4 &viewProj) {
        glm::vec4 rowX = glm::row(viewProj, 0);
        glm::vec4 rowY = glm::row(viewProj, 1);
        glm::vec4 rowZ = glm::row(viewProj, 2);
        glm::vec4 rowW = glm::row(viewProj, 3);

        planes[0] = normalize(rowW + rowX);
        planes[1] = normalize(rowW - rowX);
        planes[2] = normalize(rowW + rowY);
        planes[3] = normalize(rowW - rowY);
        planes[4] = normalize(rowW + rowZ);
        planes[5] = normalize(rowW - rowZ);
    }

    inline bool intersects(const glm::vec3 &minPoint, const glm::vec3 &maxPoint) const {
        for (int i = 0; i<6; i++) { 
            if (planes[i].x * minPoint.x + planes[i].y * maxPoint.y + planes[i].z * minPoint.z + planes[i].w > 0) continue; 
            if (planes[i].x * minPoint.x + planes[i].y * maxPoint.y + planes[i].z * maxPoint.z + planes[i].w > 0) continue;
            if (planes[i].x * maxPoint.x + planes[i].y * maxPoint.y + planes[i].z * maxPoint.z + planes[i].w > 0) continue;
            if (planes[i].x * maxPoint.x + planes[i].y * maxPoint.y + planes[i].z * minPoint.z + planes[i].w > 0) continue;
            if (planes[i].x * maxPoint.x + planes[i].y * minPoint.y + planes[i].z * minPoint.z + planes[i].w > 0) continue;
            if (planes[i].x * maxPoint.x + planes[i].y * minPoint.y + planes[i].z * maxPoint.z + planes[i].w > 0) continue;
            if (planes[i].x * minPoint.x + planes[i].y * minPoint.y + planes[i].z * maxPoint.z + planes[i].w > 0) continue;
            if (planes[i].x * minPoint.x + planes[i].y * minPoint.y + planes[i].z * minPoint.z + planes[i].w > 0) continue;
            return false;
        }
        return true;
    }

    private:
    glm::vec4 planes[6];
};

}