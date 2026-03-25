#version 450

layout (location = 0) in vec3 frag_color;
layout (location = 1) in vec2 frag_uv;
layout (location = 0) out vec4 out_color;

layout (set = 1, binding = 0) uniform sampler2D albedo_tex;

layout (push_constant) uniform PushConstants {
    layout (offset = 64) vec4 albedo_tint;
} pc;

void main() {
    vec4 tex_color = texture(albedo_tex, frag_uv);
    out_color = tex_color * vec4(frag_color, 1.0) * pc.albedo_tint;
}
