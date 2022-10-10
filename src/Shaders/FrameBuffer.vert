#version 460 core
layout (location = 0) in vec2 aPos; // only vec2 as it is just a quad
layout (location = 1) in vec2 atexCoords; 

out vec2 texCoords;

void main() 
{
	gl_Position = vec4(aPos, 0.0, 1.0);
	texCoords = atexCoords;
}