#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 5) in mat4 aInstanceMatrix;

layout (location = 0) out VS_TO_FS {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 Normal;
    vec3 ViewDir;
    // things in tangent space
    mat3 TBN;
} vs_out;

layout (std140, binding = 0) uniform Matrices{
    mat4 projview;
    vec3 viewPos;
};

void main() {
    vs_out.FragPos = vec3(aInstanceMatrix * vec4(aPos, 1.0));
    vs_out.TexCoords = aTexCoords;
    mat3 normalMatrix = transpose(inverse(mat3(aInstanceMatrix)));
    vs_out.Normal = normalMatrix * aNormal;
    vs_out.ViewDir = normalize(viewPos - vs_out.FragPos);

    // generate the TBN (Tangent, Bitangent, Normal) matrix by transforming all the vectors to the coordinate system we'd like to work in (which in this case is world-space)
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 N = normalize(normalMatrix * aNormal);
    
    // Gram-Schmidt process
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    mat3 TBN = mat3(T, B, N);

    vs_out.TBN = TBN;

    gl_Position = projview * vec4(vs_out.FragPos, 1.0);
}