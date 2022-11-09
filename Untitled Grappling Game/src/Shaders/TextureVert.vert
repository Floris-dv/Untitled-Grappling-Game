#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

layout (location = 0) out VS_TO_FS {
    vec2 TexCoords;
    vec3 Normal;
} vs_out;

layout (binding = 0, std140) uniform Matrices{
    mat4 projview;
    vec3 viewPos;
};

layout (location = 0) uniform mat4 model;

void main() {
	vs_out.TexCoords = aTexCoords;
    vs_out.Normal = aNormal;
    gl_Position = projview * model * vec4(aPos, 1.0);
}