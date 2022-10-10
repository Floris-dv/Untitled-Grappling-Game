#version 460 core

#define USE_HDR 0
#define ENABLE_BLOOM 1
#define CORRECT_GAMMA 1

out vec4 fragColor;

in vec2 texCoords;

uniform sampler2D screen;
#if ENABLE_BLOOM
uniform sampler2D bloomBlur;

uniform bool bloom;
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
    fragColor.rgb = col;
}