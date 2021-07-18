#include "tech-core/shader/shader.hpp"
#include "tech-core/shader/stage.hpp"
#include "tech-core/shader/shader_builder.hpp"

namespace Engine {


Shader::Shader(const ShaderBuilder &builder)
    : name(builder.name),
    vertexStage(builder.vertexStage),
    fragmentStage(builder.fragmentStage) {

}


bool Shader::isCompatibleWith(const PipelineRequirements &requirements) const {
    if (fragmentStage && !fragmentStage->isCompatibleWith(requirements)) {
        return false;
    }

    if (vertexStage && !vertexStage->isCompatibleWith(requirements)) {
        return false;
    }

    // FIXME: Currently assuming that the standard stages will be compatible. We could be asking about a custom pipeline
    return true;
}

std::vector<ShaderVariable> Shader::getVariables() const {
    std::vector<ShaderVariable> variables;
    if (vertexStage) {
        auto otherVariables = vertexStage->getVariables();
        variables.insert(variables.end(), otherVariables.begin(), otherVariables.end());
    }

    if (fragmentStage) {
        auto otherVariables = fragmentStage->getVariables();
        variables.insert(variables.end(), otherVariables.begin(), otherVariables.end());
    }

    return variables;
}

std::vector<ShaderVariable> Shader::getVariables(ShaderBindingUsage usage) const {
    std::vector<ShaderVariable> variables;
    if (vertexStage) {
        auto otherVariables = vertexStage->getVariables();
        for (auto &var : otherVariables) {
            if (var.usage == usage) {
                variables.push_back(var);
            }
        }
    }

    if (fragmentStage) {
        auto otherVariables = fragmentStage->getVariables();
        for (auto &var : otherVariables) {
            if (var.usage == usage) {
                variables.push_back(var);
            }
        }
    }

    return variables;
}
}