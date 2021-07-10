#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

namespace Engine {

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec4 color;
    glm::vec2 texCoord;

    static vk::VertexInputBindingDescription getBindingDescription() {
        return vk::VertexInputBindingDescription(
            0,
            sizeof(Vertex),
            vk::VertexInputRate::eVertex
        );
    }

    static std::array<vk::VertexInputAttributeDescription, 5> getAttributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription {
                0,
                0,
                vk::Format::eR32G32B32Sfloat,
                offsetof(Vertex, pos)
            },
            vk::VertexInputAttributeDescription {
                1,
                0,
                vk::Format::eR32G32B32Sfloat,
                offsetof(Vertex, normal)
            },
            vk::VertexInputAttributeDescription {
                2,
                0,
                vk::Format::eR32G32B32Sfloat,
                offsetof(Vertex, tangent)
            },
            vk::VertexInputAttributeDescription {
                3,
                0,
                vk::Format::eR32G32B32A32Sfloat,
                offsetof(Vertex, color)
            },
            vk::VertexInputAttributeDescription {
                4,
                0,
                vk::Format::eR32G32Sfloat,
                offsetof(Vertex, texCoord)
            }
        };
    }
};

}