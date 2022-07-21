#version 450

layout(binding = 0) uniform sampler2D texture_sampler;

layout(push_constant) uniform Push_constants
{
    vec2 resolution;
    vec2 mouse_position;
} constants;

layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_color;

void main()
{
    const float highlight_radius = 5.0;
    const float cursor_highlight = 1.0 - step(highlight_radius, distance(gl_FragCoord.xy, floor(constants.mouse_position.xy)));
    out_color = texture(texture_sampler, in_tex_coord) + vec4(0.3) * cursor_highlight;
}
