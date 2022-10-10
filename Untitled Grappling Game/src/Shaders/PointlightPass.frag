#version 460 core

#define pi 3.1415926535 // enough precision for me
#define USE_BLIN_PHONG 1
#define USE_NORMAL_MAPPING 1 

#define USE_PARALLEX_MAPPING 1
#define USE_PARALLEX_OCCLUSION_MAPPING 0

layout (location = 0) out vec4 FragColor;

struct PointLight {  // offset: total size = 80 (multiple of 16)
    vec3 position;   // 0

    vec3 ambient;    // 16
    vec3 diffuse;    // 32
    vec3 specular;   // 48

    float linear;    // 64
    float quadratic; // 68
};

uniform vec3 viewPos;

uniform float shininess;
uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoSpec;
uniform sampler2D addTo;
uniform samplerCube shadowMapPoint;

uniform int index;
uniform int total;

uniform float far_plane;

uniform PointLight light;
uniform vec2 screenSize;

// from http://theorangeduck.com/page/avoiding-shader-conditionals
float when_gt(float x, float y) {
  return max(sign(x - y), 0.0);
}

float calculateSpec(vec3 lightDir, vec3 normal, vec3 viewDir) {
#if USE_BLIN_PHONG
    // blin-phong lighting model
    vec3 halfWayDir = normalize(lightDir + viewDir);
    float spec = max(dot(halfWayDir, normal), 0.0);

    // this model doesn't conserve energy, which I'd like to have, so someone at https://www.rorydriscoll.com/2009/01/25/energy-conservation-in-games/
    // has figured out the normalization factor to be (n+8)/(8*pi);
    float energyConservation = (8.0 + shininess) / (8.0 * pi);
#else
    // phong lighting model
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = clamp(dot(viewDir, reflectDir), 0.0, 1.0);
    float energyConservation = (2.0 + material.shininess) / (2.0 * pi);
#endif
    spec = /*energyConservation * */ pow(spec, shininess);

    return spec;
}

float calculateShadowPoint(vec3 fragPos, vec3 lightPos, in samplerCube s)
{
    // transform the frag pos to a position relative to the lights position
    vec3 fragToLight = fragPos - lightPos;

    float closestDepth = texture(s, fragToLight).r; 
    // transform closestDepth into range [0, far_plane]
    closestDepth *= far_plane;

    // get the current depth by calculating the distance between the fragPos and the lightPos
    float currentDepth = length(fragToLight);


    float bias = 0.001;
    float shadow = when_gt(closestDepth, currentDepth - bias);

    // TODO: do PCF
    return shadow;
}

void main() {
    vec2 texCoords = gl_FragCoord.xy / screenSize;

    gl_FragDepth = index / total;
    vec3 fragPos = texture(gPosition, texCoords).rgb + viewPos;
    vec3 diffuseTex = texture(gAlbedoSpec, texCoords).rgb;
    vec3 normal = texture(gNormal, texCoords).rgb;

    // attenuation
    float d = distance(light.position, fragPos);
    float attenuation = 1.0 / (1.0 + (light.linear * d) + (light.quadratic * d * d));

    vec3 ambient = light.ambient * diffuseTex;

    // diffuse 
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0); // how much to diffuse

    vec3 diffuse = light.diffuse * diff * diffuseTex;

    // specular
    float spec = calculateSpec(lightDir, normal, normalize(fragPos - viewPos));
    vec3 specular = light.specular * spec * texture(gAlbedoSpec, texCoords).a;

    // shadow
    float shadow = calculateShadowPoint(fragPos, light.position, shadowMapPoint);
    FragColor = vec4(texture(addTo, texCoords).rgb + (ambient + (diffuse + specular) * shadow) * attenuation, 1.0);
    // FragColor = vec4(1.0, 0.0, 0.0, 0.0);// + FragColor;
}