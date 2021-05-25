#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include "forward.hpp"

namespace Engine {

void draw(const BoundingSphere &, uint32_t colour = 0xFFFFFFFF);
void draw(const BoundingBox &, uint32_t colour = 0xFFFFFFFF);
void draw(const Frustum &, uint32_t colour = 0xFFFFFFFF);
void draw(const Plane &, uint32_t colour = 0xFFFFFFFF);

void drawLine(const glm::vec3 &from, const glm::vec3 &to, uint32_t colour = 0xFFFFFFFF);
void drawAABB(const glm::vec3 &from, const glm::vec3 &to, uint32_t colour = 0xFFFFFFFF);
void drawPlane(const glm::vec4 &plane, uint32_t colour = 0xFFFFFFFF);
void drawPlane(const glm::vec4 &plane, const glm::vec2 &size, uint32_t colour = 0xFFFFFFFF);

void drawGizmoAxis(const glm::vec3 &origin);
}
