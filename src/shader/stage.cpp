#include "tech-core/shader/stage.hpp"
#include "tech-core/shader/stage_builder.hpp"
#include <iostream>
#include <vulkanutils.hpp>

namespace Engine {

bool areTypesSimilar(const SpvReflectTypeDescription &type1, const SpvReflectTypeDescription &type2);
std::optional<ShaderValueType> convertType(const SpvReflectTypeDescription &type);

bool ShaderStage::isCompatibleWith(const ShaderStage &other) const {
    switch (type) {
        case ShaderStageType::Fragment:
            if (other.type == ShaderStageType::Vertex) {
                return areInputsCompatibleWithModule(other);
            } else if (other.type == ShaderStageType::Fragment) {
                return false;
            }
            break;
        case ShaderStageType::Vertex:
            if (other.type == ShaderStageType::Fragment) {
                return other.areInputsCompatibleWithModule(*this);
            } else if (other.type == ShaderStageType::Vertex) {
                return false;
            }
            break;
    }

    return true;
}

bool ShaderStage::isCompatibleWith(const PipelineRequirements &pipeline) const {
    switch (type) {
        case ShaderStageType::Fragment:
            return areOutputsCompatibleWithPipeline(pipeline);
        case ShaderStageType::Vertex:
            return areInputsCompatibleWithPipeline(pipeline);
    }

    return true;
}

bool ShaderStage::areInputsCompatibleWithModule(const ShaderStage &stage) const {
    uint32_t inputCount;
    moduleInfo.EnumerateInputVariables(&inputCount, nullptr);

    std::vector<SpvReflectInterfaceVariable *> inputVariables(inputCount);
    moduleInfo.EnumerateInputVariables(&inputCount, inputVariables.data());

    for (auto inputIndex = 0; inputIndex < inputCount; ++inputIndex) {
        auto input = inputVariables[inputIndex];
        auto &inputType = *input->type_description;

        SpvReflectResult result;
        auto output = stage.moduleInfo.GetOutputVariableByLocation(input->location, &result);
        if (result == SPV_REFLECT_RESULT_SUCCESS) {
            auto &outputType = *output->type_description;
            if (!areTypesSimilar(inputType, outputType)) {
                // Location matches but type is not compatible so stage is not compatible
                return false;
            }
        } else {
            // Output didnt exist
            return false;
        }
    }

    return true;
}

bool ShaderStage::areOutputsCompatibleWithPipeline(const PipelineRequirements &pipeline) const {
    assert(type == ShaderStageType::Fragment);

    auto &requiredOutputs = pipeline.getOutputAttachments();

    for (auto &requiredOutput : requiredOutputs) {
        SpvReflectResult result;
        auto output = moduleInfo.GetOutputVariableByLocation(requiredOutput.location, &result);
        if (result == SPV_REFLECT_RESULT_SUCCESS) {
            auto outputType = convertType(*output->type_description);

            if (!outputType) {
                std::cerr << "Unknown output type in SPIRV shader:" << std::endl;
                std::cerr << " Op: " << output->type_description->op << std::endl;
                auto &traits = output->type_description->traits;
                std::cerr << " Traits.Numeric.Scalar: " << std::endl;
                std::cerr << "   width: " << traits.numeric.scalar.width << std::endl;
                std::cerr << "   signedness: " << traits.numeric.scalar.signedness << std::endl;
                std::cerr << " Traits.Numeric.Vector: " << std::endl;
                std::cerr << "   comp. count: " << traits.numeric.vector.component_count << std::endl;
                std::cerr << " Traits.Numeric.Matrix: " << std::endl;
                std::cerr << "   stride: " << traits.numeric.matrix.stride << std::endl;
                std::cerr << "   row count: " << traits.numeric.matrix.row_count << std::endl;
                std::cerr << "   col count: " << traits.numeric.matrix.column_count << std::endl;
                std::cerr << " Type Flags: " << output->type_description->type_flags << std::endl;
                std::cerr << " Storage Class: " << output->type_description->storage_class << std::endl;
                std::cerr << " Type name: " << output->type_description->type_name << std::endl;
                return false;
            }

            if (requiredOutput.valueType != outputType) {
                return false;
            }
        } else {
            return false;
        }
    }

    return true;
}

bool ShaderStage::areInputsCompatibleWithPipeline(const PipelineRequirements &pipeline) const {
    assert(type == ShaderStageType::Vertex);

    auto &availableInputs = pipeline.getVertexDefinition();

    uint32_t inputCount;
    moduleInfo.EnumerateInputVariables(&inputCount, nullptr);

    std::vector<SpvReflectInterfaceVariable *> requiredInputs(inputCount);
    moduleInfo.EnumerateInputVariables(&inputCount, requiredInputs.data());

    for (auto input : requiredInputs) {
        auto inputType = convertType(*input->type_description);

        if (!inputType) {
            std::cerr << "Unknown input type in SPIRV shader:" << std::endl;
            std::cerr << " Op: " << input->type_description->op << std::endl;
            auto &traits = input->type_description->traits;
            std::cerr << " Traits.Numeric.Scalar: " << std::endl;
            std::cerr << "   width: " << traits.numeric.scalar.width << std::endl;
            std::cerr << "   signedness: " << traits.numeric.scalar.signedness << std::endl;
            std::cerr << " Traits.Numeric.Vector: " << std::endl;
            std::cerr << "   comp. count: " << traits.numeric.vector.component_count << std::endl;
            std::cerr << " Traits.Numeric.Matrix: " << std::endl;
            std::cerr << "   stride: " << traits.numeric.matrix.stride << std::endl;
            std::cerr << "   row count: " << traits.numeric.matrix.row_count << std::endl;
            std::cerr << "   col count: " << traits.numeric.matrix.column_count << std::endl;
            std::cerr << " Type Flags: " << input->type_description->type_flags << std::endl;
            std::cerr << " Storage Class: " << input->type_description->storage_class << std::endl;
            std::cerr << " Type name: " << input->type_description->type_name << std::endl;
            return false;
        }

        bool found = false;
        for (auto &requiredInput : availableInputs) {
            if (requiredInput.location != input->location) {
                continue;
            }

            if (requiredInput.valueType != inputType) {
                // Location matches but type is not compatible so stage is not compatible
                return false;
            } else {
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }
    }

    return true;
}

ShaderStage::ShaderStage(const ShaderStageBuilder &builder)
    : shaderData(builder.shaderData),
    type(builder.type),
    entrypoint(builder.entrypoint),
    specializationData(builder.specializationData),
    specializationEntries(builder.specializationEntries),
    moduleInfo(builder.shaderData),
    variables(builder.variables),
    systemInputs(builder.systemInputs) {

    assert(moduleInfo.GetResult() == SPV_REFLECT_RESULT_SUCCESS);

    for (auto &var : variables) {
        var.second.stage = type;
    }
}

std::vector<uint8_t> ShaderStage::reassignBindings(const std::unordered_map<uint32_t, uint32_t> &bindingSets) const {
    SpvReflectResult result;

    spv_reflect::ShaderModule module(shaderData);
    assert(moduleInfo.GetResult() == SPV_REFLECT_RESULT_SUCCESS);

    uint32_t bindingCount;
    result = module.EnumerateDescriptorBindings(&bindingCount, nullptr);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    std::vector<SpvReflectDescriptorBinding *> descriptorBindings(bindingCount);
    module.EnumerateDescriptorBindings(&bindingCount, descriptorBindings.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    auto getBindingById = [&descriptorBindings](uint32_t id, uint32_t *currentSet) -> SpvReflectDescriptorBinding * {
        for (auto current : descriptorBindings) {
            if (current->binding == id) {
                if (currentSet) {
                    *currentSet = current->set;
                }
                return current;
            }
        }

        return nullptr;
    };

    bool modified = false;
    for (auto &pair : bindingSets) {
        uint32_t sourceSet;
        auto bindingId = pair.first;
        auto targetSet = pair.second;

        auto binding = getBindingById(bindingId, &sourceSet);

        if (binding && sourceSet != targetSet) {
            result = module.ChangeDescriptorBindingNumbers(
                binding, SPV_REFLECT_BINDING_NUMBER_DONT_CHANGE, targetSet
            );
            assert(result == SPV_REFLECT_RESULT_SUCCESS);
            std::cout << "[ShaderStage] Moved binding " << bindingId << " from set " << sourceSet << " to "
                << targetSet << std::endl;
            modified = true;
        }
    }

    if (modified) {
        std::vector<uint8_t> data(module.GetCodeSize());
        std::memcpy(data.data(), module.GetCode(), module.GetCodeSize());

        return data;
    } else {
        return shaderData;
    }
}

vk::ShaderModule ShaderStage::createShaderModule(
    vk::Device device, const std::unordered_map<uint32_t, uint32_t> &bindingSets,
    vk::PipelineShaderStageCreateInfo &createInfo, vk::SpecializationInfo &specInfo
) const {
    auto data = reassignBindings(bindingSets);

    auto module = device.createShaderModule(
        {{}, static_cast<uint32_t>(data.size()), reinterpret_cast<const uint32_t *>(data.data()) }
    );

    specInfo = {
        vkUseArray(specializationEntries), sizeof(uint32_t) * specializationData.size(),
        specializationData.data()
    };

    vk::ShaderStageFlagBits stage;
    switch (type) {
        case ShaderStageType::Vertex:
            stage = vk::ShaderStageFlagBits::eVertex;
            break;
        case ShaderStageType::Fragment:
            stage = vk::ShaderStageFlagBits::eFragment;
            break;
        default:
            throw std::runtime_error("Invalid shader stage");
    }

    createInfo = {
        {}, stage, module, entrypoint.data(), &specInfo
    };

    return module;
}

std::vector<ShaderVariable> ShaderStage::getVariables() const {
    std::vector<ShaderVariable> vars(variables.size());
    size_t index = 0;
    for (auto &pair : variables) {
        vars[index++] = pair.second;
    }

    return vars;
}

bool areTypesSimilar(const SpvReflectTypeDescription &type1, const SpvReflectTypeDescription &type2) {
    if (type1.type_flags != type2.type_flags) {
        return false;
    }
    if (type1.decoration_flags != type2.decoration_flags) {
        return false;
    }
    if (type1.member_count != type2.member_count) {
        return false;
    }

    for (auto memberIndex = 0; memberIndex < type1.member_count; ++memberIndex) {
        auto &member1 = type1.members[memberIndex];
        auto &member2 = type2.members[memberIndex];

        if (!areTypesSimilar(member1, member2)) {
            return false;
        }
    }

    return true;
}

std::optional<ShaderValueType> convertType(const SpvReflectTypeDescription &type) {
    switch (type.op) {
        case SpvOpTypeVector:
            if (type.traits.numeric.scalar.width == 32) {
                switch (type.traits.numeric.vector.component_count) {
                    case 2:
                        return ShaderValueType::Vec2;
                    case 3:
                        return ShaderValueType::Vec3;
                    case 4:
                        return ShaderValueType::Vec4;
                }
            } else if (type.traits.numeric.scalar.width == 64) {
                switch (type.traits.numeric.vector.component_count) {
                    case 2:
                        return ShaderValueType::DVec2;
                    case 3:
                        return ShaderValueType::DVec3;
                    case 4:
                        return ShaderValueType::DVec4;
                }
            }
            break;
        case SpvOpTypeVoid:
            return ShaderValueType::Void;
        case SpvOpTypeBool:
            return ShaderValueType::Bool;
        case SpvOpTypeInt:
            if (type.traits.numeric.scalar.signedness) {
                return ShaderValueType::Int;
            } else {
                return ShaderValueType::Uint;
            }
        case SpvOpTypeFloat:
            if (type.traits.numeric.scalar.width == 32) {
                return ShaderValueType::Float;
            } else if (type.traits.numeric.scalar.width == 64) {
                return ShaderValueType::Double;
            }
            break;
        default:
            break;
    }

    return {};
}


}
