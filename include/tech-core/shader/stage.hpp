#pragma once

#include "common.hpp"
#include "requirements.hpp"
#include "../forward.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <spirv_reflect.h>

namespace Engine {

class ShaderStage {
    friend class PipelineBuilder;
public:
    explicit ShaderStage(const ShaderStageBuilder &);

    ShaderStageType getType() const { return type; };

    bool isCompatibleWith(const ShaderStage &) const;
    bool isCompatibleWith(const PipelineRequirements &) const;

    std::vector<ShaderVariable> getVariables() const;

    const std::unordered_map<uint32_t, ShaderSystemInput> &getSystemInputs() const { return systemInputs; }

private:
    std::vector<uint8_t> shaderData;
    std::string entrypoint;

    ShaderStageType type { ShaderStageType::Vertex };

    std::vector<uint32_t> specializationData;
    std::vector<vk::SpecializationMapEntry> specializationEntries;

    std::unordered_map<uint32_t, ShaderVariable> variables;
    std::unordered_map<uint32_t, ShaderSystemInput> systemInputs;

    spv_reflect::ShaderModule moduleInfo;

    bool areInputsCompatibleWithModule(const ShaderStage &) const;
    bool areOutputsCompatibleWithPipeline(const PipelineRequirements &) const;
    bool areInputsCompatibleWithPipeline(const PipelineRequirements &) const;

    vk::ShaderModule createShaderModule(
        vk::Device device, vk::PipelineShaderStageCreateInfo &createInfo, vk::SpecializationInfo &specInfo
    ) const;
};

}