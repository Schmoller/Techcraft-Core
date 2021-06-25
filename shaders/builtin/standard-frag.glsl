#version 450
#pragma shader_stage(fragment)
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec4 fragColour;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragTexCoord;
//layout(set = 3, binding = 2) uniform sampler2DArray texSampler;

void main() {
    //    outColor = texture(texSampler, fragTexCoord) * vec4(fragColour, 1.0);
    outColor = fragColour;
}