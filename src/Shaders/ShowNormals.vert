#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;  

out VS_TO_GEOM {
	vec3 normal;
    vec2 texCoords;
    mat3 TBN;
} vs_out;


uniform mat4 model;
uniform mat4 view;

void main() {

    mat3 normalMatrix = mat3(transpose(inverse(view * model)));
    vs_out.normal = normalize(normalMatrix * aNormal);
    vs_out.texCoords = aTexCoords;
    vec3 T = normalize(vec3(model * vec4(aTangent, 1.0)));
    vec3 N = normalize(vec3(model * vec4(aNormal, 1.0)));

    // Gram-Schmidt process
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    vs_out.TBN = mat3(T, B, N);

    gl_Position = view * model * vec4(aPos, 1.0);
}