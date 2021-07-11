#include "tech-core/shader/shader_builder.hpp"
#include "tech-core/shader/shader.hpp"
#include "tech-core/shader/stage.hpp"

namespace Engine {

ShaderBuilder::ShaderBuilder(std::string name)
    : name(std::move(name)) {

}

ShaderBuilder &ShaderBuilder::withStage(std::shared_ptr<ShaderStage> stage) {
    assert(stage);

    switch (stage->getType()) {
        case ShaderStageType::Vertex:
            if (fragmentStage && !fragmentStage->isCompatibleWith(*stage)) {
                throw std::runtime_error("Vertex shader is not compatible with the current fragment shader");
            }

            vertexStage = std::move(stage);
            break;
        case ShaderStageType::Fragment:
            if (vertexStage && !vertexStage->isCompatibleWith(*stage)) {
                throw std::runtime_error("Fragment shader is not compatible with the current vertex shader");
            }

            fragmentStage = std::move(stage);
            break;
        default:
            assert(false);
    }

    return *this;
}

std::shared_ptr<Shader> ShaderBuilder::build() const {
    // Empty shaders make no sense
    assert(vertexStage || fragmentStage);

    // TODO: If we dont provide both vertex and fragment stages, then we will later on fill these with the standard
    // shaders. It would be nice to ensure that the shaders that have been provided will work with at least one standard
    // pipeline.

    if (vertexStage && fragmentStage) {
        assert(vertexStage->isCompatibleWith(*fragmentStage));
    }

    return std::make_shared<Shader>(*this);
}

}