#version 450

// -----------------------------------------------------------------------
// Inputs from vertex shader
// -----------------------------------------------------------------------
layout(location = 0) in vec3 frag_world_pos;
layout(location = 1) in vec3 frag_world_normal;
layout(location = 2) in vec3 frag_color;
layout(location = 3) in vec2 frag_uv;

// -----------------------------------------------------------------------
// Set 0 — CameraData (same block as vertex shader)
// -----------------------------------------------------------------------
layout(set = 0, binding = 0) uniform CameraData {
    mat4  view;
    mat4  proj;
    mat4  view_proj;
    vec3  camera_pos;
    float _pad;
} camera;

// -----------------------------------------------------------------------
// Set 1 — per-material albedo texture
// -----------------------------------------------------------------------
layout(set = 1, binding = 0) uniform sampler2D albedo_texture;

// -----------------------------------------------------------------------
// Push constants — albedo tint at offset 64 (after the mat4 model).
// The layout(offset=64) directive tells GLSL where this field starts
// in the push constant block relative to the block's beginning.
// -----------------------------------------------------------------------
layout(push_constant) uniform PushConstants {
    layout(offset = 64) vec4 albedo_tint;
} push;

// -----------------------------------------------------------------------
// Output
// -----------------------------------------------------------------------
layout(location = 0) out vec4 out_color;

// -----------------------------------------------------------------------
// Simple single directional light — hardcoded for Phase 3.
// Phase 4 replaces this with a full PBR BRDF and a light UBO.
// -----------------------------------------------------------------------
const vec3  LIGHT_DIR   = normalize(vec3(1.0, 2.0, 1.5));
const vec3  LIGHT_COLOR = vec3(1.0, 0.95, 0.90);
const float AMBIENT     = 0.12;

void main()
{
    vec4 albedo = texture(albedo_texture, frag_uv)
                * vec4(frag_color, 1.0)
                * push.albedo_tint;

    vec3  N       = normalize(frag_world_normal);
    float n_dot_l = max(dot(N, LIGHT_DIR), 0.0);
    float light   = AMBIENT + (1.0 - AMBIENT) * n_dot_l;

    out_color = vec4(albedo.rgb * LIGHT_COLOR * light, albedo.a);
}
