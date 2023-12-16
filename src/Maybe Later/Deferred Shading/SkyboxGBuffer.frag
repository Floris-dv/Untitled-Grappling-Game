#version 460 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 texCoords;

uniform vec3 viewPos;

uniform samplerCube skyBox;

void main() {
	gPosition = texCoords;
	gNormal = texCoords - viewPos;
	gAlbedoSpec = vec4(texture(skyBox, texCoords).rgb, 0.0);
}