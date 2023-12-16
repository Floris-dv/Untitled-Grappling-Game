#version 460 core
// Just Framebuffer.vert, but it only renders stuff in the middle
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

// 1/(size 0-1) x y
layout (location = 1) uniform vec2 uSize;

layout (location = 0) out vec2 texCoords;

void main() {
	gl_Position = vec4(aPos*uSize, 0.0, 1.0);
	texCoords = aTexCoords;
}