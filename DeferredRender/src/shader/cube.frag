#version 450

layout(location = 0) in  vec3 frag_color;
layout(location = 1) in  vec2 frag_uv;
layout(location = 0) out vec4 out_color;

void main() {
    // Checkerboard pattern using UV - replace with a texture sample in Phase 3b
    float checker = mod(floor(frag_uv.x * 8.0) + floor(frag_uv.y * 8.0), 2.0);
    vec3  pattern = mix(frag_color, frag_color * 0.5, checker);
    out_color = vec4(pattern, 1.0);
}