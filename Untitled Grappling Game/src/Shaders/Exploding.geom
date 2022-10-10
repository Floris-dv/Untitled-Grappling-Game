#version 460 core

layout (triangles) in;
layout (triangle_strip, max_vertices = 3) out;

in VS_FOR_FS {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} pass_through[];

out VS_TO_FS {
	vec3 FragPos;
	vec3 Normal;
	vec2 TexCoords;
} fs;

uniform float time;

vec3 GetNormal() {
	vec3 a = vec3(gl_in[0].gl_Position) - vec3(gl_in[1].gl_Position);
	vec3 b = vec3(gl_in[2].gl_Position) - vec3(gl_in[1].gl_Position);

	return normalize(cross(a, b));
}

void main() {
	float explosionTime = exp(time);

	vec3 normal = GetNormal();

	fs.FragPos = pass_through[0].FragPos;
	fs.Normal = pass_through[0].Normal;
	fs.TexCoords = pass_through[0].TexCoords;

	gl_Position = gl_in[0].gl_Position + vec4(normal * explosionTime, 0.0);
	EmitVertex();

	fs.FragPos = pass_through[1].FragPos;
	fs.Normal = pass_through[1].Normal;
	fs.TexCoords = pass_through[1].TexCoords;
	
	gl_Position = gl_in[1].gl_Position + vec4(normal * explosionTime, 0.0);
	EmitVertex();

	fs.FragPos = pass_through[2].FragPos;
	fs.Normal = pass_through[2].Normal;
	fs.TexCoords = pass_through[2].TexCoords;

	gl_Position = gl_in[2].gl_Position + vec4(normal * explosionTime, 0.0);
	EmitVertex();
	
	EndPrimitive();
};