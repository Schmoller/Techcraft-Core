#version 450 core
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 fColor;

layout(set=0, binding=0) uniform sampler2DArray sTexture;

layout(location = 0) in struct {
    vec4 Color;
    vec2 UV;
} In;

layout(push_constant) uniform PushConstant {
    layout(offset = 16) uint arrayIndex;
} pc;

void main()
{
    fColor = In.Color * texture(sTexture, vec3(In.UV.st, pc.arrayIndex));
}
