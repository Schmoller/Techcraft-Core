#version 450
#pragma shader_stage(vertex)
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec2 outUV;

// From: https://github.com/SaschaWillems/Vulkan/blob/master/data/shaders/glsl/bloom/gaussblur.vert
void main() {
    outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(outUV * 2.0f - 1.0f, 0.0f, 1.0f);
}