#version 330

out vec4 color;

flat in vec3 center_pos;
flat in vec3 particle_color;

void main() {
    vec2 c = vec2((2.0 / 720.0) * gl_FragCoord.x - (1280.0 / 720.0),
                  (2.0 / 720.0) * gl_FragCoord.y - 1.0);

    float d = distance(center_pos.xy, c.xy);

    if (d > 0.05)
        discard;

    color = vec4(particle_color, 1.0 - d * 25.0);
}
