#version 460 core

layout (location = 0) out vec4 FragColor;

struct Material {
    sampler2D diffuse0;
    sampler2D specular0;
    sampler2D normal0;
    float shininess;
    vec3 diff0;
};

layout (location = 2) uniform Material material;

void main() {
	FragColor = vec4(material.diff0, 1.0);
}