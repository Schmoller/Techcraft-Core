#version 450
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec4 fragColour;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragTangent;
layout(location = 3) in vec2 fragTexCoord;

layout(binding = 2) uniform MaterialUbo {
    vec2 scale;
    vec2 offset;
} settings;

layout(binding = 3) uniform sampler2D albedo;
layout(binding = 4) uniform sampler2D normal;

void main() {
    outColor = texture(albedo, fragTexCoord * settings.scale + settings.offset) * fragColour;
}