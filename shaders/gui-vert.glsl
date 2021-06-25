#version 450
#pragma shader_stage(vertex)
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inTexCoord;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec4 fragColour;
layout(location = 1) out vec3 fragTexCoord;

layout(push_constant) uniform Transform {
    mat4 view;
} transform;

void main() {
    gl_Position = transform.view * vec4(inPosition, 1.0);
    fragColour = inColor;
    fragTexCoord = inTexCoord;
}