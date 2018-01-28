#version 330

out vec4 color;

flat in vec3 center_pos;
flat in vec3 particle_color;
flat in float particle_scale;

void main() {
    vec2 c = vec2((2.0 / 720.0) * gl_FragCoord.x - (1280.0 / 720.0),
                  (2.0 / 720.0) * gl_FragCoord.y - 1.0);

    const float falloff = 0.025;
    const float hardness = 5.5;
    float d = distance(center_pos.xy, c.xy);
    float a = 1.0f;

    float s = particle_scale + falloff / 2;

    if (d > s)
        discard;

    if (d > (s - falloff))  {
        float fd = (d - (s - falloff)) / falloff;
        a *= 1.0 - pow(fd * hardness, 3.0);
    }

    color = vec4(particle_color, a);
}
