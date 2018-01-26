#version 330

out vec4 color;

flat in vec3 center_pos;
flat in vec3 particle_color;
flat in float particle_scale;

void main() {
    vec2 c = vec2((2.0 / 720.0) * gl_FragCoord.x - (1280.0 / 720.0),
                  (2.0 / 720.0) * gl_FragCoord.y - 1.0);

    const float f = 0.02;
    float d = distance(center_pos.xy, c.xy);
    float a = 1.0f;

    if (d > particle_scale)
        discard;

    if (d > (particle_scale - f))  {
        float fd = (d - (particle_scale - f)) / f;
        a *= 1.0 - pow(fd, 2.0);
    }

    color = vec4(particle_color, a);
}
