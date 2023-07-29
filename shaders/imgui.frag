#version 450 core
layout(location = 0) out vec4 fColor;

layout(set=0, binding=0) uniform sampler2D sTexture;

layout(location = 0) in struct {
    vec4 Color;
    vec2 UV;
} In;


vec3 linear_to_srgb(vec3 linear_rgb) {
    bvec3 cutoff = lessThan(linear_rgb, vec3(0.0031308));
    vec3  higher = vec3(1.055) * pow(linear_rgb, vec3(1.0 / 2.4)) - vec3(0.055);
    vec3  lower	 = linear_rgb * vec3(12.92);

    return mix(higher, lower, cutoff);
}

// Converts a color from sRGB gamma to linear light gamma
vec3 srgb_to_linear(vec3 sRGB) {
    bvec3 cutoff = lessThan(sRGB, vec3(0.04045));
    vec3  higher = pow((sRGB + vec3(0.055)) / vec3(1.055), vec3(2.4));
    vec3  lower	 = sRGB / vec3(12.92);

    return mix(higher, lower, cutoff);
}

void main()
{
    vec4 tex_read = texture(sTexture, In.UV.st);
    fColor = vec4(srgb_to_linear(In.Color.rgb), In.Color.a) * vec4(srgb_to_linear(tex_read.rgb), tex_read.a);
}
