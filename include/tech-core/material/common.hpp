#pragma once

#include <string>
#include <glm/vec2.hpp>

namespace Engine {

struct MaterialScaleAndOffset {
    glm::vec2 scale { 1, 1 };
    glm::vec2 offset { 0, 0 };
};

}
