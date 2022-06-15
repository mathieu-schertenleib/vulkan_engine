#version 450

layout(binding = 1) uniform sampler2D tex_sampler;

layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec2 frag_tex_coord;
layout(location = 2) in vec2 resolution;
layout(location = 3) in vec2 mouse_position;

layout(location = 0) out vec4 out_color;

void main()
{
    const float min_dimension = min(resolution.x, resolution.y);
    const float mouse_distance = distance(mouse_position, gl_FragCoord.xy);
    const float scale = 1.0 - 0.5 * smoothstep(min_dimension * 0.25, min_dimension * 0.5, mouse_distance);
    out_color = texture(tex_sampler, frag_tex_coord) * vec4(frag_color, 1.0) * scale;
}