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

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 frag_tex_coord;
layout(location = 2) out vec2 resolution;
layout(location = 3) out vec2 mouse_position;


#define PI 3.141592741
#define FREQUENCY 0.2

void main()
{
    gl_Position = vec4(in_position, 1.0);

    const float r = 0.5 + 0.5 * sin(2.0 * PI * FREQUENCY * ubo.time);
    const float g = 0.5 + 0.5 * sin(2.0 * PI * FREQUENCY * ubo.time + PI * 2.0 / 3.0);
    const float b = 0.5 + 0.5 * sin(2.0 * PI * FREQUENCY * ubo.time + PI * 4.0 / 3.0);
    frag_color = vec3(r, g, b);

    frag_tex_coord = in_tex_coord;

    resolution = ubo.resolution;
    mouse_position = ubo.mouse_position;
}