
// from http://theorangeduck.com/page/avoiding-shader-conditionals
float when_gt(float x, float y) {
  return max(sign(x - y), 0.0);
}

uniform float far_plane;

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
