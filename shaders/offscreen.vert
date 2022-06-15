#version 450

layout(binding = 0) uniform Uniform_buffer_object
{
    vec2 resolution;
    vec2 mouse_position;
    float time;
} ubo;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_tex_coord;

layout(location = 0) out vec2 out_tex_coord;

void main()
{
    const float angle = 6.28 * 0.1 * ubo.time;
    const mat2 rotate = mat2(cos(angle), sin(angle), -sin(angle), cos(angle));
    gl_Position = vec4(rotate * in_position.xy, 0.0, 1.0);
    out_tex_coord = in_tex_coord;
}