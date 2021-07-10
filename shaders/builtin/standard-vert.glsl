#version 450
#pragma shader_stage(vertex)
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
} cam;

layout(set = 1, binding = 1) uniform EntityUBO {
    mat4 transform;
} obj;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec4 inColor;
layout(location = 4) in vec2 inTexCoord;

layout(location = 0) out vec4 fragColour;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragTangent;
layout(location = 3) out vec2 fragTexCoord;
layout(location = 4) out vec4 fragPosition;


void main() {
    fragPosition = obj.transform * vec4(inPosition, 1.0);
    gl_Position = cam.proj * cam.view * fragPosition;
    fragNormal = normalize(inNormal * mat3(obj.transform));
    fragTangent = normalize(inTangent * mat3(obj.transform));
    fragTexCoord = inTexCoord;
    fragColour = inColor;

}