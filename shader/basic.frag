#version 460 core
#extension GL_KHR_vulkan_glsl:enable

layout (location = 0) in vec4 texColor;
layout (location = 1) in vec2 texCoord;

layout (location = 0) out vec4 FragColor;

layout (set = 0, binding = 0) uniform sampler2D Tex;

void main(){
	FragColor = texColor;
}