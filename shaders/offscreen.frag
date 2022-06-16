#version 450

layout(binding = 1) uniform sampler2D texture_sampler;

layout(location = 0) in vec2 in_tex_coord;
layout(location = 1) in vec2 in_resolution;
layout(location = 2) in vec2 in_mouse_position;
layout(location = 3) in float in_time;

layout(location = 0) out vec4 out_color;

void main()
{
    const vec2 frag_uv = gl_FragCoord.xy / in_resolution.xy;
    const vec2 mouse_uv = in_mouse_position.xy / in_resolution.xy;
    const float r = clamp(distance(frag_uv, mouse_uv), 0.0, 1.0);
    out_color = texture(texture_sampler, in_tex_coord, 1.0) * vec4(5.0 / (1.0 + 50 * r * r));
}