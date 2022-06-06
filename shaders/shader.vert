#version 450

layout(binding = 0) uniform Uniform_buffer_object
{
    vec2 pos;
    float time;
} ubo;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_tex_coord;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 frag_tex_coord;

void main()
{
    gl_Position = vec4(in_position, 1.0);
    frag_color = vec3(0.0, 0.0, 0.0);
    frag_tex_coord = in_tex_coord;
}