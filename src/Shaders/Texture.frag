#version 460 core

layout (location = 0) out vec4 FragColor;

in VS_TO_FS {
    vec2 TexCoords;
    vec3 Normal;
} fs_in;

struct Material {
    sampler2D diffuse0;
};


struct DirLight {   // offset: total size = 64
    vec3 direction; // 0
    
    vec3 ambient;   // 16
    vec3 diffuse;   // 32
    vec3 specular;  // 48
};

struct PointLight {  // offset: total size = 80 (multiple of 16)
    vec3 position;   // 0

    vec3 ambient;    // 16
    vec3 diffuse;    // 32
    vec3 specular;   // 48

    float linear;    // 64
    float quadratic; // 68
};

struct SpotLight {     // offset: total size = 96
    vec3 position;     // 0
    vec3 direction;    // 16

    vec3 ambient;      // 32
    vec3 diffuse;      // 48
    vec3 specular;     // 64
    
    float linear;      // 80
    float quadratic;   // 84

    // NEEDS TO BE A COSINE
    float cutOff;      // 88
    float outerCutOff; // 92
};

#define numPointLights 4

layout (std140) uniform Lights {
    DirLight dirLight; // 0
    PointLight pointLights[numPointLights]; // +64; +144; +224; +304
    SpotLight SL; // + 384
    // total: 480 bytes, = 124 floats
};

uniform SpotLight spotLight; 

uniform Material material;

uniform vec3 viewPos;

void main() {
    FragColor = vec4(texture(material.diffuse0, fs_in.TexCoords).rgb * dirLight.diffuse * max(dot(normalize(dirLight.direction), fs_in.Normal), 0.2), 1.0);
}