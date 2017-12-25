#version 330

layout(location = 0) in vec3 vert_pos;
layout(location = 1) in vec3 pos_offset;
layout(location = 2) in vec3 color;
layout(location = 3) in float scale;

flat out vec3 center_pos;
flat out vec3 particle_color;
flat out float particle_scale;

void main()
{
    center_pos = pos_offset;
    particle_color = color;
    particle_scale = scale;

    gl_Position.xyz = (vert_pos * scale + pos_offset);
    gl_Position.x *= (720.0 / 1280.0);
    gl_Position.w = 1.0;
}
