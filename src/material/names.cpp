#include "tech-core/material/names.hpp"

namespace Engine {

const std::string_view MaterialVariables::AlbedoTexture = "Albedo";
const std::string_view MaterialVariables::NormalTexture = "Normal";
const std::string_view MaterialVariables::RoughnessTexture = "Roughness";
const std::string_view MaterialVariables::OcclusionTexture = "Occlusion";
const std::string_view MaterialVariables::MetalnessTexture = "Metalness";
const ShaderUniformId<MaterialScaleAndOffset> MaterialVariables::ScaleAndOffsetUniform = { "ScaleAndOffset" };

}