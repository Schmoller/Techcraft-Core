#pragma once

#include "../forward.hpp"
#include <memory>

namespace Engine::BuiltIn {

extern std::shared_ptr<ShaderStage> StandardPipelineVertexStage;
extern std::shared_ptr<ShaderStage> StandardPipelineFSFragmentStage;
extern std::shared_ptr<ShaderStage> StandardPipelineDSFragmentStage;

extern std::shared_ptr<Shader> StandardPipelineForwardShading;
extern std::shared_ptr<Shader> StandardPipelineDSGeometryPass;

}