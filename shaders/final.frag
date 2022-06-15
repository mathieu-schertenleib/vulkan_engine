#version 450

layout(binding = 0) uniform sampler2D offscreen_image_sampler;

layout(location = 0) in vec2 in_tex_coord;

layout(location = 0) out vec4 out_color;

void main()
{
    out_color = texture(offscreen_image_sampler, in_tex_coord);
}
