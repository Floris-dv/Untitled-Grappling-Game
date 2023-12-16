#version 460 core
layout (location = 0) in vec3 aPos;

layout (location = 0) uniform mat4 model;

layout (binding = 0, std140) uniform Matrices{
    mat4 projview;
	vec3 viewPos;
};

void main() 
{
	gl_Position = projview * model * vec4(aPos, 1.0);
}
