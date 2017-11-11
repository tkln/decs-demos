#version 330

layout(location = 0) in vec2 vert_pos;
layout(location = 1) in vec2 tex_coord;

out vec2 tex_coord_frag;

void main(void)
{
    /* TODO don't hardcode the viewport size */
    gl_Position = vec4(vert_pos.x * 2.0 / 1280.0 - 1.0,
                       vert_pos.y * -(2.0 / 720.0) + 1.0, 0, 1);
    tex_coord_frag = tex_coord;
}
