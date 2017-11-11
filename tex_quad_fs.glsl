#version 330

in vec2 tex_coord_frag;

uniform sampler2D tex;

out vec4 frag_color;

void main(void)
{
    frag_color = texture(tex, tex_coord_frag);
}
