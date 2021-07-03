#version 450
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec4 fragColour;
layout(location = 1) in vec2 fragTexCoord;
layout(binding = 2) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler, fragTexCoord);
    outColor *= fragColour;
}