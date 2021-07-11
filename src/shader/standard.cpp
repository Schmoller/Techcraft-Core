#include "tech-core/shader/standard.hpp"

namespace Engine::BuiltIn {

std::shared_ptr<ShaderStage> StandardPipelineVertexStage;
std::shared_ptr<ShaderStage> StandardPipelineFSFragmentStage;
std::shared_ptr<ShaderStage> StandardPipelineDSFragmentStage;

std::shared_ptr<Shader> StandardPipelineForwardShading;
std::shared_ptr<Shader> StandardPipelineDSGeometryPass;

}