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
layout(location = 1) out vec2 out_resolution;
layout(location = 2) out vec2 out_mouse_position;
layout(location = 3) out float out_time;

void main()
{
    gl_Position = vec4(in_position, 1.0);
    out_tex_coord = in_tex_coord;

    out_resolution = ubo.resolution;
    out_mouse_position = ubo.mouse_position;
    out_time = ubo.time;
}