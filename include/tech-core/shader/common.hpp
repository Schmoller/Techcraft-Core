#pragma once

#include <string>

namespace Engine {

enum class ShaderStageType {
    Fragment,
    Vertex
};

enum class ShaderValueType {
    Float,
    Vec2,
    Vec3,
    Vec4,
    Int,
    IVec2,
    IVec3,
    IVec4,
    Uint,
    UVec2,
    UVec3,
    UVec4,
    Double,
    DVec2,
    DVec3,
    DVec4,
    Bool,
    Void
};

enum class ShaderBindingType {
    Texture,
    Uniform
};

enum class ShaderBindingUsage {
    Material,
    Entity,
};

struct ShaderVariable {
    std::string name;
    uint32_t bindingId;
    ShaderBindingType type;
    ShaderBindingUsage usage;
};

enum class ShaderSystemInput {
    Camera,
    Entity,
    Light
};

}