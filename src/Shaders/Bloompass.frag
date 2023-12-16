#version 460 core

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

layout (location = 0) in vec2 texCoords;
layout (binding = 0) uniform sampler2D screen;

void main() {
	FragColor = texture(screen, texCoords);
	float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    BrightColor = vec4(FragColor.rgb * brightness, 1.0);
}