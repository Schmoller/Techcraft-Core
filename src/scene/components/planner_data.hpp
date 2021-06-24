#pragma once

#include "tech-core/scene/components/base.hpp"
#include "../internal.hpp"
#include <glm/mat4x4.hpp>

namespace Engine::Internal {

class PlannerData final : public Component {
public:
    explicit PlannerData(Entity &entity) : Component(entity) {};

    struct {
        EntityBuffer *buffer;
        vk::DeviceSize uniformOffset;
    } render;

    glm::mat4 absoluteTransform { 1 };
};

}