#version 460 core

layout (location = 0) out vec4 FragColor;

layout (location = 0) in vec2 texCoords;

layout (binding=0) uniform sampler2D image;

layout (location = 10) uniform bool horizontal;
float weight[3] = float[] (0.2270270270, 0.3162162162, 0.0702702703);
float offset[3] = float[](0.0, 1.3846153846, 3.2307692308);

void main()
{
    // vertical and horizontal implementation from https://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
    vec2 tex_offset = 1.0 / textureSize(image, 0); // gets size of single texel
    vec3 result = texture(image, texCoords).rgb * weight[0];
    if(horizontal)
    {
        for(int i = 1; i < 3; ++i)
        {
            result += texture(image, texCoords + vec2(tex_offset.x * i * offset[i], 0.0)).rgb * weight[i];
            result += texture(image, texCoords - vec2(tex_offset.x * i * offset[i], 0.0)).rgb * weight[i];
        }
     }
     else
     {
        for(int i = 1; i < 3; ++i)
        {
            result += texture(image, texCoords + vec2(0.0, tex_offset.y * i * offset[i])).rgb * weight[i];
            result += texture(image, texCoords - vec2(0.0, tex_offset.y * i * offset[i])).rgb * weight[i];
        }
     }
     FragColor = vec4(result, 1.0);
}