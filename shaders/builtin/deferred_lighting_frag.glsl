#version 450
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable

layout (input_attachment_index = 0, binding = 3) uniform subpassInput inDiffuseOcclusion;
layout (input_attachment_index = 1, binding = 4) uniform subpassInput inNormalRoughness;
layout (input_attachment_index = 2, binding = 5) uniform subpassInput inDepth;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 diffuseOcclusion = subpassLoad(inDiffuseOcclusion);
    outColor = vec4(diffuseOcclusion.rgb, 1.0);
}