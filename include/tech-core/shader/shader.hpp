#pragma once

#include "../forward.hpp"
#include "requirements.hpp"
#include <memory>

namespace Engine {

class Shader {
public:
    explicit Shader(const ShaderBuilder &);

    const std::string &getName() const { return name; }

    std::shared_ptr<ShaderStage> getVertexStage() const { return vertexStage; }

    std::shared_ptr<ShaderStage> getFragmentStage() const { return fragmentStage; };

    std::vector<ShaderVariable> getVariables() const;
    std::vector<ShaderVariable> getVariables(ShaderBindingUsage usage) const;

    bool usesStandardVertexStage() const { return !vertexStage; }

    bool usesStandardFragmentStage() const { return !fragmentStage; }

    bool isCompatibleWith(const PipelineRequirements &) const;
private:
    std::string name;

    std::shared_ptr<ShaderStage> vertexStage;
    std::shared_ptr<ShaderStage> fragmentStage;
};

}