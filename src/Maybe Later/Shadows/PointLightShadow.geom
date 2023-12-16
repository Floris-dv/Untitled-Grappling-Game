#version 460 core
layout(triangles) in;
layout (triangle_strip, max_vertices=18) out;

uniform mat4 lightSpaceMatrices[6];

out vec4 FragPos; // output from this Shader (per emitVertex)

void main() {

	for (int face = 0; face < 6; face++) {
		gl_Layer = face; // built-in variable that specifies to which face we render

		for (int i = 0; i < 3; i++) { // for each vertex
			FragPos = gl_in[i].gl_Position;
			gl_Position = lightSpaceMatrices[face] * FragPos;
			EmitVertex();
		}

		EndPrimitive();
	}
}