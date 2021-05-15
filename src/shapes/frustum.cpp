#include <tech-core/shapes/frustum.hpp>
#include <tech-core/shapes/plane.hpp>

namespace Engine {

enum ClipPlane {
    Left,
    Right,
    Top,
    Bottom,
    Near,
    Far
};


Plane Frustum::planeLeft() const {
    return Plane { planes[Left] };
}

Plane Frustum::planeRight() const {
    return Plane { planes[Right] };
}

Plane Frustum::planeTop() const {
    return Plane { planes[Top] };
}

Plane Frustum::planeBottom() const {
    return Plane { planes[Bottom] };
}

Plane Frustum::planeFar() const {
    return Plane { planes[Far] };
}

Plane Frustum::planeNear() const {
    return Plane { planes[Near] };
}

void Frustum::update(const glm::mat4 &viewProj) {
    glm::vec4 rowX = glm::row(viewProj, 0);
    glm::vec4 rowY = glm::row(viewProj, 1);
    glm::vec4 rowZ = glm::row(viewProj, 2);
    glm::vec4 rowW = glm::row(viewProj, 3);

    planes[Left] = normalize(rowW + rowX);
    planes[Right] = normalize(rowW - rowX); // right clipping plane
    planes[Bottom] = normalize(rowW + rowY);
    planes[Top] = normalize(rowW - rowY);
    planes[Near] = normalize(rowW + rowZ);
    planes[Far] = normalize(rowW - rowZ);
}
}