#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 5) in mat4 aInstanceMatrix;

void main()
{
    gl_Position = aInstanceMatrix * vec4(aPos, 1.0);
}