#pragma once

#include "../forward.hpp"
#include "requirements.hpp"
#include <memory>

namespace Engine {

class Shader {
public:
    explicit Shader(const ShaderBuilder &);

    std::shared_ptr<ShaderStage> getVertexStage() const { return vertexStage; }

    std::shared_ptr<ShaderStage> getFragmentStage() const { return fragmentStage; };

    bool usesStandardVertexStage() const { return !vertexStage; }

    bool usesStandardFragmentStage() const { return !fragmentStage; }

    bool isCompatibleWith(const PipelineRequirements &) const;
private:
    std::shared_ptr<ShaderStage> vertexStage;
    std::shared_ptr<ShaderStage> fragmentStage;
};

}