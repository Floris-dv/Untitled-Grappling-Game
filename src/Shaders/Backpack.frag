#version 460 core

#define pi 3.1415926535 // enough precision for me
#define USE_BLIN_PHONG 1
#define USE_NORMAL_MAPPING 1
#define USE_PCF 0

#define USE_PARALLEX_MAPPING 1
#define USE_PARALLEX_OCCLUSION_MAPPING 0

layout (location = 0) out vec4 FragColor;

/*
Scalar e.g. int or bool	        Each scalar has a base alignment of N.
Vector	                        Either 2N or 4N. This means that a vec3 has a base alignment of 4N.
Array of scalars or vectors	    Each element has a base alignment equal to that of a vec4.
Matrices	                    Stored as a large array of column vectors, where each of those vectors has a base alignment of vec4.
Struct	                        Equal to the computed size of its elements according to the previous rules, but padded to a multiple of the size of a vec4.
*/

struct Material {
    sampler2D diffuse0;
    sampler2D specular0;
    sampler2D normal0;
    sampler2D height0;
    float shininess;

    vec3 diff0;
    vec3 spec0;

    bool useTex;
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

in VS_TO_FS {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 Normal;
    vec4 FragPosLightSpace;
    // things in tangent space
    mat3 TBN;
} fs_in;

uniform vec3 viewPos;
uniform Material material;
uniform samplerCube skyBox;

uniform sampler2D shadowMapDir;
uniform samplerCube shadowMapPoint;

uniform float far_plane;
uniform float height_scale; // the scaling of the parallex mapping

#define numPointLights 4

layout (std140) uniform Lights {
    DirLight dirLight; // 0
    PointLight pointLights[numPointLights]; // +64; +144; +224; +304
    SpotLight SL; // + 384
    // total: 480 bytes, = 124 floats
};

uniform SpotLight spotLight; 

// global values
vec2 texCoords;
vec3 normal;
vec3 viewDir;
vec3 diffuseTex;
vec3 specularTex;

// from http://theorangeduck.com/page/avoiding-shader-conditionals
float when_gt(float x, float y) {
  return max(sign(x - y), 0.0);
}

float calculateSpec(vec3 lightDir) {
#if USE_BLIN_PHONG
    // blin-phong lighting model
    vec3 halfWayDir = normalize(lightDir + viewDir);
    float spec = max(dot(halfWayDir, normal), 0.0);

    // this model doesn't conserve energy, which I'd like to have, so someone at https://www.rorydriscoll.com/2009/01/25/energy-conservation-in-games/
    // has figured out the normalization factor to be (n+8)/(8*pi);
    float energyConservation = (8.0 + material.shininess) / (8.0 * pi);
#else
    // phong lighting model
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = clamp(dot(viewDir, reflectDir), 0.0, 1.0);
    float energyConservation = (2.0 + material.shininess) / (2.0 * pi);
#endif
    spec = /*energyConservation * */ pow(spec, material.shininess);

    return spec;
}

// uses a sampler2D
float calculateShadowDir(vec4 fragPosLightSpace, in sampler2D s)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(s, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // calculate bias (based on depth map resolution and slope)
    /*
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    */
    float bias = 0.001;
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(s, 0);
    for(int x = -1; x < 2; x++)
    {
        for(int y = -1; y < 2; y++)
        {
            float pcfDepth = texture(s, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += when_gt(currentDepth - bias, pcfDepth);
        }
    }
    shadow /= 25.0;
    
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.

    shadow *= when_gt(1.0, projCoords.z);
        
    return shadow;
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
    
    const float bias = 0.05; 

    // PCF
#if USE_PCF
    const vec3 sampleOffsetDirections[20] = vec3[]
    (
       vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
       vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
       vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
       vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
       vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
    );   

    float shadow  = 0.0;
    const int samples = 20;
    const float offset  = 0.1;
    const float diskRadius = 0.05;

    for(int i = 0; i < samples; ++i) {
        float closestDepth = texture(s, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
        closestDepth *= far_plane;   // undo mapping [0;1]
        shadow += when_gt(currentDepth - bias, closestDepth);
    }
    shadow /= (samples * samples * samples);
#else
    float shadow = when_gt(currentDepth - bias, closestDepth);
#endif

    return shadow;
}

// calculate the light from a directional light (like the sun) (includes Specular, Diffuse, and Ambient lighting)
// normal & viewdir need to be normalized for this to work
vec3 calcDirLight(DirLight light, in sampler2D s) {
    vec3 ambient = light.ambient * diffuseTex;

    // diffuse 
    vec3 lightDir = normalize(light.direction);
    float diff = max(dot(normal, lightDir), 0.0); // how much to diffuse

    vec3 diffuse = light.diffuse * diff * diffuseTex;
    
    // specular
    float spec = calculateSpec(lightDir);
    vec3 specular = light.specular * spec * specularTex;
    // no antenuation

    // shadow:
    float shadow = calculateShadowDir(fs_in.FragPosLightSpace, s);

    return ambient + (1.0 - shadow) * (diffuse + specular);  
}

// calculate the light from a pointlight (or: a simple lamp) (includes Specular, Diffuse, and Ambient lighting)
// normal & viewdir need to be normalized for this to work
vec3 calcPointLight(PointLight light, vec3 fragPos) {
    // attenuation
    float d = distance(light.position, fragPos);
    float attenuation = 1.0 / (1.0 + (light.linear * d) + (light.quadratic * d * d));

    vec3 ambient = light.ambient * diffuseTex;

    // diffuse 
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0); // how much to diffuse

    vec3 diffuse = light.diffuse * diff * diffuseTex;

    // specular
    float spec = calculateSpec(lightDir);

    vec3 specular = light.specular * spec * specularTex;

    float shadow = calculateShadowPoint(fragPos, light.position, shadowMapPoint);

    return ambient + (diffuse + specular) * (1.0 - shadow) * attenuation;
}


// calculate the light from a spotlight (includes Specular, Diffuse, and Ambient lighting)
// normal & viewdir need to be normalized for this to work
vec3 calcSpotLight(SpotLight light, SpotLight sl, vec3 fragPos) {
    // attenuation
    float d = length(sl.position - fragPos);
    float attenuation = 1.0 / (1.0 + sl.linear * d + sl.quadratic * (d * d));

    vec3 lightDir = normalize(sl.position - fragPos);
    // spotlight intensity
    float theta = dot(lightDir, normalize(-sl.direction)); 
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    
    vec3 ambient = sl.ambient * diffuseTex;
    vec3 diffuse = sl.diffuse * diff * diffuseTex;
    
    // there is a problem with this implementation: that the model can be lit from behind,
    // the fix is to make spec 0 if diff is 0 (as that is 0 if the model is lit from behind)
    // but as doing if branches is shaders is slow, i'm saving a little time with the funtion when_gt from http://theorangeduck.com/page/avoiding-shader-conditionals
    float spec =  calculateSpec(lightDir) * when_gt(diff, 0);

    // combine results
    vec3 specular = sl.specular * spec * specularTex;

    return (diffuse + specular) * attenuation * intensity + ambient * attenuation;
}

// the near and far values from the projection matrix:
float near = 0.1;
float far = 100.0;

// gl_FragCoord is nonLinear, to linearize it use this function
float LinearizeDepth(float depth) 
{
    float z = depth * 2.0 - 1.0; // back to NDC 
    return (2.0 * near * far) / (far + near - z * (far - near));	
}

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
    viewDir = normalize(viewPos - fs_in.FragPos);
#if USE_PARALLEX_MAPPING
    // offset texture coords with parallex mapping:
    texCoords = fs_in.TexCoords;
    texCoords = ParallaxMapping(texCoords, viewDir);
    // there sometimes are weird border artifacts, as the texcoords can sample outside of the range [0,1], solution: discard it then
    if(texCoords.x > 1.0 || texCoords.y > 1.0 || texCoords.x < 0.0 || texCoords.y < 0.0)
        discard;
#else
    texCoords = fs_in.TexCoords;
#endif
#if USE_NORMAL_MAPPING
    // retrieve normal from normal map in range [0,1]
    normal = texture(material.normal0, texCoords).rgb;

    // transform normal vector to range [-1, 1]
    normal = normal * 2.0 - 1.0;
    normal = normalize(fs_in.TBN * normal);
#else
    normal = fs_in.Normal;
#endif
    if (material.useTex) {
        diffuseTex = vec3(texture(material.diffuse0, texCoords));
        specularTex = vec3(texture(material.specular0, texCoords));
    }
    else {
        diffuseTex = material.diff0;
        specularTex = material.spec0;
    }

    vec3 res = calcDirLight(dirLight, shadowMapDir);

    for (int i = 0; i < numPointLights; i++)
       res += calcPointLight(pointLights[i], fs_in.FragPos);
    

    res += calcSpotLight(spotLight, SL, fs_in.FragPos);

    FragColor = vec4(res, 1.0);
    
    /*
    if(brightness > 1.0)
        BrightColor = vec4(res, 1.0);
    else
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    vec3 R = reflect(-viewDir, norm);
    
    FragColor = mix(vec4(texture(skyBox, R).rgb, 1.0), FragColor, 0.8);
    */
    
    // Cool for a Horror game: FragColor -= vec4(vec3(gl_FragCoord.z), 0.0);
}