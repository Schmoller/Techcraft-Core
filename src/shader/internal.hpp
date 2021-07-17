#pragma once

#include "tech-core/shader/common.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vulkan/vulkan.hpp>

namespace Engine::Internal {

enum class ShaderBindingUpdateFrequency {
    OnlyOnce,
    PerFrame,
    PerMaterial,
    PerEntity,
    Custom
};

struct ShaderBindingGroup {
    ShaderBindingUpdateFrequency frequency;
    std::unordered_set<uint32_t> bindings;
};

std::unordered_map<uint32_t, ShaderBindingGroup> allocateBindingSets(
    std::unordered_map<uint32_t, ShaderVariable> &variableBindings,
    std::unordered_map<uint32_t, ShaderSystemInput> &systemInputs
);

}