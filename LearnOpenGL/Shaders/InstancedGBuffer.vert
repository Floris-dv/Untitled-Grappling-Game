#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;

layout (location = 5) in mat4 aInstanceMatrix; // location attributes are: 3, 4, 5, 6 (as the maximum amount of data allowed for a vertex attribute is a vec4)



layout (std140) uniform Matrices{
    mat4 projview;
};

out vec3 FragPos;
out vec2 TexCoords;
out vec3 Normal;

out mat3 TBN;

void main()
{
    FragPos = vec3(aInstanceMatrix * vec4(aPos, 1.0));
    TexCoords = aTexCoords;
    Normal = aNormal;

    mat3 normalMatrix = transpose(inverse(mat3(aInstanceMatrix)));
    // generate the TBN (Tangent, Bitangent, Normal) matrix by transforming all the vectors to the coordinate system we'd like to work in (which in this case is world-space)
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 N = normalize(normalMatrix * aNormal);
    
    // Gram-Schmidt process
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    TBN = mat3(T, B, N);

    gl_Position = projview * vec4(FragPos, 1.0);
}