#version 450
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable
//
//struct TextureManip {
//    vec2 scale;
//    vec2 offset;
//};

layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outNormalRoughness;
layout(location = 2) out vec4 outDiffuseOcclusion;

layout(location = 0) in vec4 fragColour;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragTangent;
layout(location = 3) in vec2 fragTexCoord;
layout(location = 4) in vec4 fragPosition;

//
//layout(set = 1, binding = 2) uniform TextureUbo {
//    TextureManip albedo;
//    TextureManip normal;
//} textureSettings;

layout(set = 2, binding = 3) uniform sampler2D albedo;
layout(set = 3, binding = 4) uniform sampler2D normal;

vec3 computeNormal() {
    vec3 tangentNormal = texture(normal, fragTexCoord).xyz * 2.0 - 1.0;

    vec3 worldNormal = normalize(fragNormal);
    vec3 worldTangent = normalize(fragTangent);
    vec3 worldBiTangent = normalize(cross(worldNormal, worldTangent));
    mat3 tangentToWorldTransform = mat3(worldTangent, worldBiTangent, worldNormal);
    return normalize(tangentToWorldTransform * tangentNormal);
}

void main() {
    vec4 color = texture(albedo, fragTexCoord) * fragColour;
    vec3 normal = computeNormal();

    outPosition = fragPosition;
    outDiffuseOcclusion = vec4(color.rgb, 0);// TODO: Occlusion
    outNormalRoughness = vec4(normal, 0);// TODO: Roughness
}