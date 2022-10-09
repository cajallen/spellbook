#version 450 core
layout(location = 0) out vec4 fColor;

layout(set=0, binding=0) uniform sampler2D sTexture;

layout(location = 0) in struct {
    vec4 Color;
    vec2 UV;
} In;

void main()
{
    fColor = vec4(pow(In.Color.rgb, vec3(2.2)), In.Color.a) * texture(sTexture, In.UV.st);
}
