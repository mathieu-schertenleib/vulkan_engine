#version 450

layout(binding = 0) uniform Uniform_buffer_object
{
    vec2 resolution;
    vec2 mouse_position;
    float time;
    float delta_time;
} ubo;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_tex_coord;

layout(location = 0) out vec2 frag_tex_coord;

void main()
{
    gl_Position = vec4(in_position, 1.0);
    frag_tex_coord = in_tex_coord;
}