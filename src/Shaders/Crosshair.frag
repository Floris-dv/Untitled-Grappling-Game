#version 460 core
layout (location = 0) out vec4 fragColor;

layout(location = 0) in vec2 texCoords;

layout(binding = 1) uniform sampler2D crosshairTex;

void main() {
	fragColor = texture(crosshairTex, texCoords);
}