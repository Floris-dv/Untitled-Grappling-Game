#version 460 core

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

in VS_TO_GEOM {
	vec3 normal;
	vec2 texCoords;
    mat3 TBN;
} gs_in[];

struct Material {
    sampler2D diffuse0;
    sampler2D specular0;
    sampler2D normal0;
    float shininess;
    vec3 diff0;
};

uniform Material material;

uniform float Magnitude;

uniform mat4 proj;

uniform vec3 viewPos;

void GenerateNormal(int index) {
	vec3 viewDir = normalize(vec3(gl_in[index].gl_Position) - viewPos);
	vec3 normal = texture(material.normal0, gs_in[index].texCoords).rgb;
	// transform normal vector to range [-1, 1]
	normal = normalize(normal * 2.0 - 1.0);
    
	// set the direction right
	normal = normalize(gs_in[index].TBN * normal);


	if (dot(viewDir, normal) > 0)
		return;
	gl_Position = proj * gl_in[index].gl_Position;
	EmitVertex();

	gl_Position = proj * (gl_in[index].gl_Position + vec4(normal, 1.0) * Magnitude);
	EmitVertex();

	EndPrimitive();
}

void main() {
	GenerateNormal(0);
	GenerateNormal(1);
	GenerateNormal(2);
};