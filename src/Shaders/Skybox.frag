#version 460 core
out vec4 FragColor;

in vec3 texCoords;

uniform samplerCube skyBox;

void main() {
	FragColor = texture(skyBox, texCoords);
}
