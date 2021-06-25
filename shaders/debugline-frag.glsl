#version 450
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable

/*
Simple debug line drawer. This is a simple passthrough of the color.
*/

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec4 fragColor;

void main() {
    outColor = fragColor;
}