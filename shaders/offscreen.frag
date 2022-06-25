#version 450

layout(binding = 1) uniform sampler2D texture_sampler;

layout(location = 0) in vec2 in_tex_coord;
layout(location = 1) in vec2 in_resolution;
layout(location = 2) in vec2 in_mouse_position;
layout(location = 3) in float in_time;

layout(location = 0) out vec4 out_color;

void main()
{
    const float highlight_radius = 5.0;
    const float cursor_highlight = 1.0 - step(highlight_radius, distance(gl_FragCoord.xy, floor(in_mouse_position.xy)));
    out_color = texture(texture_sampler, in_tex_coord) + vec4(0.3) * cursor_highlight;
}
