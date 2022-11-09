#version 460 core
layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec3 texCoords;

layout (binding = 0) uniform samplerCube skyBox;

void main() {
	FragColor = texture(skyBox, texCoords);
}
