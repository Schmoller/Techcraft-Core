#include <tech-core/debug.hpp>
#include <tech-core/shapes/bounding_sphere.hpp>
#include <tech-core/shapes/frustum.hpp>
#include <tech-core/shapes/bounding_box.hpp>
#include <tech-core/shapes/plane.hpp>

#include <tech-core/subsystem/debug.hpp>

#include <glm/glm.hpp>

namespace Engine {

enum class Axis {
    X,
    Y,
    Z
};

template<Axis T>
void drawRing(const glm::vec3 &origin, float radius, uint32_t colour) {
    auto inst = Subsystem::DebugSubsystem::instance();

    glm::vec3 last;

    auto circumference = 2 * M_PI * radius;

    uint32_t steps = std::min(std::max(static_cast<int>(circumference / 10), 8), 30);

    if constexpr (T == Axis::X) {
        last = origin + glm::vec3 { 0.0f, radius, 0.0f };
    } else if constexpr (T == Axis::Y) {
        last = origin + glm::vec3 { radius, 0.0f, 0.0f };
    } else {
        last = origin + glm::vec3 { radius, 0.0f, 0.0f };
    }

    for (auto i = 1; i <= steps; ++i) {
        auto angle = (static_cast<float>(i) / static_cast<float>(steps)) * M_PI * 2;

        glm::vec3 pos;

        if constexpr (T == Axis::X) {
            pos = origin + glm::vec3 { 0.0f, std::cos(angle) * radius, std::sin(angle) * radius };
        } else if constexpr (T == Axis::Y) {
            pos = origin + glm::vec3 { std::cos(angle) * radius, 0.0f, std::sin(angle) * radius };
        } else {
            pos = origin + glm::vec3 { std::cos(angle) * radius, std::sin(angle) * radius, 0.0f };
        }

        inst->debugDrawLine(last, pos, colour);

        last = pos;
    }
}

void draw(const BoundingSphere &sphere, uint32_t colour) {
    glm::vec3 origin { sphere.x, sphere.y, sphere.z };
    auto radius = sphere.radius;
    auto radiusX2 = radius * 2;

    uint32_t rings = std::min(std::max(static_cast<int>(radiusX2 / 50), 1), 20);
    // Dont consider the start and end to be valid rings but need the spacing
    auto ringOffset = radiusX2 / static_cast<float>(rings + 2);

    // draw rings
    auto xOrigin = origin - glm::vec3 { radius, 0, 0 };
    auto yOrigin = origin - glm::vec3 { 0, radius, 0 };
    auto zOrigin = origin - glm::vec3 { 0, 0, radius };
    for (auto i = 1; i < rings + 2; ++i) {
        auto height = ringOffset * i;
        auto ringRadius = std::sqrt(radiusX2 * height - (height * height));
        drawRing<Axis::X>(xOrigin + glm::vec3 { height, 0, 0 }, ringRadius, colour);
        drawRing<Axis::Y>(yOrigin + glm::vec3 { 0, height, 0 }, ringRadius, colour);
        drawRing<Axis::Z>(zOrigin + glm::vec3 { 0, 0, height }, ringRadius, colour);
    }
}

void draw(const BoundingBox &box, uint32_t colour) {
    auto inst = Subsystem::DebugSubsystem::instance();
    inst->debugDrawBox(box, colour);
}

void draw(const Frustum &frustum, uint32_t colour) {
    auto inst = Subsystem::DebugSubsystem::instance();

    // Work out intersections
    auto planeNear = frustum.planeNear();
    auto planeFar = frustum.planeFar();
    auto planeLeft = frustum.planeLeft();
    auto planeRight = frustum.planeRight();
    auto planeTop = frustum.planeTop();
    auto planeBottom = frustum.planeBottom();

    auto nearTL = planeNear.intersect(planeLeft, planeTop);
    auto nearTR = planeNear.intersect(planeRight, planeTop);
    auto nearBL = planeNear.intersect(planeLeft, planeBottom);
    auto nearBR = planeNear.intersect(planeRight, planeBottom);

    auto farTL = planeFar.intersect(planeLeft, planeTop);
    auto farTR = planeFar.intersect(planeRight, planeTop);
    auto farBL = planeFar.intersect(planeLeft, planeBottom);
    auto farBR = planeFar.intersect(planeRight, planeBottom);

    inst->debugDrawLine(nearTL, nearTR, colour);
    inst->debugDrawLine(nearTL, nearBL, colour);
    inst->debugDrawLine(nearTR, nearBR, colour);
    inst->debugDrawLine(nearBL, nearBR, colour);

    inst->debugDrawLine(farTL, farTR, colour);
    inst->debugDrawLine(farTL, farBL, colour);
    inst->debugDrawLine(farTR, farBR, colour);
    inst->debugDrawLine(farBL, farBR, colour);

    inst->debugDrawLine(nearTL, farTL, colour);
    inst->debugDrawLine(nearTR, farTR, colour);
    inst->debugDrawLine(nearBL, farBL, colour);
    inst->debugDrawLine(nearBR, farBR, colour);
}

void draw(const Plane &plane, uint32_t colour) {
    drawPlane(plane.getEquation(), colour);
}

void drawLine(const glm::vec3 &from, const glm::vec3 &to, uint32_t colour) {
    auto inst = Subsystem::DebugSubsystem::instance();
    inst->debugDrawLine(from, to, colour);
}

void drawAABB(const glm::vec3 &from, const glm::vec3 &to, uint32_t colour) {
    auto inst = Subsystem::DebugSubsystem::instance();
    inst->debugDrawBox(from, to, colour);
}

void drawPlane(const glm::vec4 &plane, uint32_t colour) {
    drawPlane(plane, { 5, 5 }, colour);
}

void drawPlane(const glm::vec4 &plane, const glm::vec2 &size, uint32_t colour) {
    auto inst = Subsystem::DebugSubsystem::instance();

    glm::vec3 normal = glm::vec3 { plane.x, plane.y, plane.z };
    float dist = -plane.w;

    glm::vec3 up;
    // Dont get a degenerate axis
    if (normal.x == 0 && normal.y == 0 && normal.z == 1) {
        up = { 0, -1, 0 };
    } else {
        up = { 0, 0, 1 };
    }

    // FIXME: This doesnt work

    // NOTE: This is incorrect. This does not render the plane as it should be
    // Generate a rectangle
    glm::vec3 origin { -normal.x * dist, -normal.y * dist, -normal.z * dist };
//    glm::vec3 origin = -normal * dist;
    inst->debugDrawLine({}, origin, 0xFF00FF00);
    glm::vec3 axis1 = glm::normalize(glm::cross(normal, up));
    glm::vec3 axis2 = glm::normalize(glm::cross(axis1, normal));

    glm::vec3 cornerTopRight = origin + axis1 * size.x + axis2 * size.y;
    glm::vec3 cornerTopLeft = origin - axis1 * size.x + axis2 * size.y;
    glm::vec3 cornerBottomRight = origin + axis1 * size.x - axis2 * size.y;
    glm::vec3 cornerBottomLeft = origin - axis1 * size.x - axis2 * size.y;

    // Draw
    inst->debugDrawLine(cornerTopRight, cornerBottomRight, colour);
    inst->debugDrawLine(cornerBottomRight, cornerBottomLeft, colour);
    inst->debugDrawLine(cornerBottomLeft, cornerTopLeft, colour);
    inst->debugDrawLine(cornerTopLeft, cornerTopRight, colour);
}

void drawGizmoAxis(const glm::vec3 &origin) {
    auto inst = Subsystem::DebugSubsystem::instance();
    inst->debugDrawLine(
        origin,
        origin + glm::vec3 { 10, 0, 0 },
        0xFFFF0000
    );
    inst->debugDrawLine(
        origin,
        origin + glm::vec3 { 0, 10, 0 },
        0xFF00FF00
    );
    inst->debugDrawLine(
        origin,
        origin + glm::vec3 { 0, 0, 10 },
        0xFF0000FF
    );
}

};