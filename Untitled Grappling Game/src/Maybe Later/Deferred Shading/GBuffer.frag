#version 460 core
#define USE_PARALLEX_MAPPING 0
#define USE_PARALLEX_OCCLUSION_MAPPING 0

#define USE_NORMAL_MAPPING 1

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec2 TexCoords;
in vec3 FragPos;
in vec3 Normal;

in mat3 TBN;

uniform float height_scale;

struct Material {
    sampler2D diffuse0;
    sampler2D specular0;
    sampler2D normal0;
    sampler2D height0;
    float shininess;
};

uniform Material material;

uniform vec3 viewPos;

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir) {
#if USE_PARALLEX_OCCLUSION_MAPPING
    // there isn't much displacement going on when looking at a surface straight on, so do less sammples then
    const float minLayers = 8.0;
    const float maxLayers = 32.0;
    float numLayers = mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0));

    float sizeLayer = 1 / numLayers; // size of each layer
    float currentLayerDepth = 0.0;
    // calculate the amount to shift the texture coords per layer:
    vec2 P = viewDir.xy /* / viewDir.z */ * height_scale;
    vec2 dTexCoords = P / numLayers;

    
    // get initial values
    vec2  currentTexCoords     = texCoords;

    float currentDepthMapValue = texture(material.height0, currentTexCoords).r;
    float lastDepthMapValue = 1.0;

    while (currentLayerDepth < currentDepthMapValue) {
        lastDepthMapValue = currentDepthMapValue;
        // shift the texture coords along the direction of P
        currentTexCoords -= dTexCoords;
        // get the depthmap value at the current texture coords
        currentDepthMapValue = texture(material.height0, currentTexCoords).r;
        // get the depth of the next layer
        currentLayerDepth += sizeLayer;
    }

    vec2 prevTexCoords = currentTexCoords + dTexCoords;
    
    // get depth after and before collision for linear interpolation
    float afterDepth = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = lastDepthMapValue - (currentLayerDepth + sizeLayer);

    // interpolation
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = mix(currentTexCoords, prevTexCoords, weight);

    return finalTexCoords;

#else
    float height =  texture(material.height0, texCoords).r; 
    return texCoords - viewDir.xy /* / viewDir.z */ * (height * height_scale);  
#endif
}


void main()
{
    vec3 viewDir = normalize(viewPos - FragPos);

#if USE_PARALLEX_MAPPING
    // offset texture coords with parallex mapping:
    vec2 texCoords = TexCoords;
    texCoords = ParallaxMapping(texCoords, viewDir);
    // there sometimes are weird border artifacts, as the texcoords can sample outside of the range [0,1], solution: discard it then
    if(texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
        discard;
#else
    vec2 texCoords = TexCoords;
#endif

#if USE_NORMAL_MAPPING
    // retrieve normal from normal map in range [0,1]
    vec3 normal = texture(material.normal0, texCoords).rgb;

    // transform normal vector to range [-1, 1]
    normal = normal * 2.0 - 1.0;
    gNormal = normalize(TBN * normal);
#else
    gNormal = normalize(Normal);
#endif

    // store the fragment position vector in the first gbuffer texture
    gPosition = FragPos - viewPos;
    // and the diffuse per-fragment color
    gAlbedoSpec.rgb = texture(material.diffuse0, texCoords).rgb;
    // store specular intensity in gAlbedoSpec's alpha component
    gAlbedoSpec.a = texture(material.specular0, texCoords).r;
}