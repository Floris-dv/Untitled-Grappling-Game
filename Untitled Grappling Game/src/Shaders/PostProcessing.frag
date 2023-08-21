#version 460 core

#define USE_HDR 1
#define ENABLE_BLOOM 1
#define CORRECT_GAMMA 0

layout (location = 0) out vec4 fragColor;

layout (location = 0) in vec2 texCoords;

layout (binding = 0) uniform sampler2D screen;
#if ENABLE_BLOOM
layout (binding = 1) uniform sampler2D bloomBlur;

layout (location = 2) uniform bool bloom;
#endif

void main()
{
    vec3 col = texture(screen, texCoords).rgb;

    if(bloom)
        col += texture(bloomBlur, texCoords).rgb; // additive blending

#if USE_HDR
#if CORRECT_GAMMA
    // Unreal 3 tonemapping, Documentation: "Color Grading"
    // Adapted to be close to ACES, with similar range
    // Gamma 2.2 correction is baked in
    col = col/(col+0.155f) * 1.019f;
#else
    // ACES tone mapping: Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    col = clamp((col * (a * col + b)) / (col * (c * col + d) + e), 0.0, 1.0);
#endif
#endif
    fragColor = vec4(col, 1.0);
}