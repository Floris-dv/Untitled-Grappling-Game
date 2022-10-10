#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 5) in mat4 aInstanceMatrix; // location attributes are: 3, 4, 5, 6 (as the maximum amount of data allowed for a vertex attribute is a vec4)

uniform mat4 lightSpaceMatrix;

out VS_TO_FS {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 Normal;
    vec4 FragPosLightSpace;
    // things in tangent space
    mat3 TBN;
} vs_out;

layout (std140) uniform Matrices{
    mat4 projview;
};

void main()
{
    vs_out.FragPos = vec3(aInstanceMatrix * vec4(aPos, 1.0));
    vs_out.TexCoords = aTexCoords;
    vs_out.FragPosLightSpace = lightSpaceMatrix * vec4(vs_out.FragPos, 1.0);
    vs_out.Normal = aNormal;

    mat3 normalMatrix = transpose(inverse(mat3(aInstanceMatrix)));
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