#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform sampler2D texSampler[2];

layout(push_constant) uniform PER_OBJECT
{
    layout(offset = 192) uint index;
}tex;

//layout(location = 0) in vec3 fragColor;
layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    //outColor = texture(texSampler, fragTexCoord);
    outColor = texture(texSampler[tex.index], fragTexCoord);
}