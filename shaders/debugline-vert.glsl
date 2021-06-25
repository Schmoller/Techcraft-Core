#version 450
#pragma shader_stage(vertex)
#extension GL_ARB_separate_shader_objects : enable

/*
Simple debug line drawer which does not use a vertex buffer, builds the line from a push constant
*/

layout(binding = 0) uniform CameraUBO {
    mat4 view;
    mat4 proj;
} cam;

layout(push_constant) uniform DebugLine {
    vec4 from;
    vec4 to;
    vec4 color;
} line;

layout(location = 0) out vec4 fragColour;

void main() {
    vec4 inPosition;
    if (gl_VertexIndex == 0) {
        inPosition = line.from;
    } else {
        inPosition = line.to;
    }

    gl_Position = cam.proj * cam.view * inPosition;
    fragColour = line.color;
}