#version 450
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable
//
//struct TextureManip {
//    vec2 scale;
//    vec2 offset;
//};

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec4 fragColour;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
//
//layout(set = 1, binding = 2) uniform TextureUbo {
//    TextureManip albedo;
//    TextureManip normal;
//} textureSettings;

layout(set = 2, binding = 3) uniform sampler2D albedo;
layout(set = 3, binding = 4) uniform sampler2D normal;

void main() {
    outColor = texture(albedo, fragTexCoord) * fragColour;
}