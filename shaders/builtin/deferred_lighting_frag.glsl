#version 450
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable

#define LT_DIRECTIONAL 0
#define LT_POINT 1
#define LT_SPOT 2

const vec3 attenuation = vec3(0.02f, 0.01f, 0.04f);

layout (input_attachment_index = 0, binding = 3) uniform subpassInput inPosition;
layout (input_attachment_index = 1, binding = 4) uniform subpassInput inNormalRoughness;
layout (input_attachment_index = 2, binding = 5) uniform subpassInput inDiffuseOcclusion;
layout (input_attachment_index = 3, binding = 6) uniform subpassInput inDepth;

layout (set = 1, binding = 2) uniform LightUBO {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float range;
    uint type;
} light;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 position = subpassLoad(inPosition);
    vec4 normalRoughness = subpassLoad(inNormalRoughness);
    vec4 diffuseOcclusion = subpassLoad(inDiffuseOcclusion);

    vec3 normal = normalize(normalRoughness.xyz);

    if (light.type == LT_DIRECTIONAL) {
        float diffuse = max(dot(normal, -light.direction), 0);
        outColor = vec4(diffuseOcclusion.rgb, 1.0) * diffuse * vec4(light.color, 1.0);
    } else if (light.type == LT_POINT) {
        vec3 toLight = light.position - position.xyz;
        float distToLight = length(toLight);

        float atten = 1.0 / dot(vec3(1, distToLight, distToLight*distToLight), attenuation / light.range);

        outColor = vec4(diffuseOcclusion.rgb, 1.0) * vec4(light.color * max(0.0, dot(normal, toLight / distToLight) * light.intensity * atten), 1);
    }
}