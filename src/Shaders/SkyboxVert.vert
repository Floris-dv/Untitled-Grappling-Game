#version 460 core
layout (location = 0) in vec3 aPos;

layout (location = 0) out vec3 texCoords;

layout (location = 1) uniform mat4 ProjunTranslatedView;

void main() {
	texCoords = aPos;
	vec4 pos = ProjunTranslatedView * vec4(aPos, 1.0);
	gl_Position = pos.xyww; // z is the depth value, and it's divided by w, so to get behind everything (= to fill the depth buffer with ones) I need to set the z component to the same value as the w component, because w / w = 1.0
}