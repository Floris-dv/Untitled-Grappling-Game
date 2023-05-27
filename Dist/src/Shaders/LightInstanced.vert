#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 5) in mat4 aModel;

layout (binding = 0, std140) uniform Matrices{
    mat4 projview;
	vec3 viewPos;
};

void main() 
{
	gl_Position = projview * aModel * vec4(aPos, 1.0);
}
