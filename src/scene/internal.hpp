#pragma once

#include <vulkan/vulkan.hpp>
#include "tech-core/forward.hpp"

namespace Engine::Internal {

struct EntityBuffer {
    uint32_t id;
    std::unique_ptr<DivisibleBuffer> buffer;
    vk::DescriptorSet set;
};

struct LightBuffer {
    uint32_t id;
    std::unique_ptr<DivisibleBuffer> buffer;
    vk::DescriptorSet set;
};

}
