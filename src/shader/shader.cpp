#include "tech-core/shader/shader.hpp"
#include "tech-core/shader/stage.hpp"
#include "tech-core/shader/shader_builder.hpp"

namespace Engine {


Shader::Shader(const ShaderBuilder &builder)
    : vertexStage(builder.vertexStage),
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
}