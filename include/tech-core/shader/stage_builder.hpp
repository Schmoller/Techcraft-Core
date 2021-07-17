#pragma once

#include "common.hpp"
#include "../forward.hpp"

#include <string>
#include <vector>
#include <memory>
#include <vulkan/vulkan.hpp>

namespace Engine {

class ShaderStageBuilder {
    friend class ShaderStage;
public:
    ShaderStageBuilder &fromBytes(const std::vector<uint8_t> &, const char *entrypoint = "main");
    ShaderStageBuilder &fromBytes(const std::vector<int8_t> &, const char *entrypoint = "main");
    ShaderStageBuilder &fromBytes(const uint8_t *, size_t size, const char *entrypoint = "main");
    ShaderStageBuilder &fromBytes(const int8_t *, size_t size, const char *entrypoint = "main");
    ShaderStageBuilder &fromFile(const char *path, const char *entrypoint = "main");
    ShaderStageBuilder &fromFile(const std::string_view &path, const char *entrypoint = "main");
    ShaderStageBuilder &withType(ShaderStageType);
    ShaderStageBuilder &withConstant(uint32_t constant, bool);
    template<typename T>
    ShaderStageBuilder &withConstant(uint32_t constant, T);
    ShaderStageBuilder &withVariable(
        std::string name, uint32_t bindingId, ShaderBindingType type,
        ShaderBindingUsage usage = ShaderBindingUsage::Material
    );
    ShaderStageBuilder &withVariable(
        const std::string_view &name, uint32_t bindingId, ShaderBindingType type,
        ShaderBindingUsage usage = ShaderBindingUsage::Material
    );
    ShaderStageBuilder &withSystemInput(ShaderSystemInput, uint32_t bindingId);

    std::shared_ptr<ShaderStage> build() const;

private:
    std::vector<uint8_t> shaderData;
    std::string entrypoint;

    ShaderStageType type { ShaderStageType::Vertex };

    std::vector<uint32_t> specializationData;
    std::vector<vk::SpecializationMapEntry> specializationEntries;
    std::unordered_map<uint32_t, ShaderVariable> variables;
    std::unordered_map<uint32_t, ShaderSystemInput> systemInputs;
};

template<typename T>
ShaderStageBuilder &ShaderStageBuilder::withConstant(uint32_t constantId, T value) {
    static_assert(sizeof(T) % sizeof(uint32_t) == 0, "Can only use types that are a multiple of 4 bytes");

    constexpr uint32_t numSlots = sizeof(T) / sizeof(uint32_t);

    uint32_t offset = sizeof(uint32_t) * specializationData.size();
    auto *interpreted = reinterpret_cast<uint32_t *>(&value);

    if constexpr (numSlots == 1) {
        specializationData.push_back(*interpreted);
    } else {
        for (uint32_t slot = 0; slot < numSlots; ++slot) {
            specializationData.push_back(interpreted[slot]);
        }
    }
    specializationEntries.emplace_back(constantId, offset, sizeof(T));

    return *this;
}

}