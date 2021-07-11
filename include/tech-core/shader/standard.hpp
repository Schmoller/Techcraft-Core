#pragma once

#include "stage.hpp"

namespace Engine {

namespace BuiltIn {

extern std::shared_ptr<ShaderStage> StandardPipelineVertexStage;
extern std::shared_ptr<ShaderStage> StandardPipelineFSFragmentStage;
extern std::shared_ptr<ShaderStage> StandardPipelineDSFragmentStage;

}

}