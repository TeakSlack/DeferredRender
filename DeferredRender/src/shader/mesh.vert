#version 450

// -----------------------------------------------------------------------
// Inputs — must match Vertex struct in vk_mesh.h (same locations/formats).
// -----------------------------------------------------------------------
layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec3 in_color;
layout(location = 3) in vec2 in_uv;

// -----------------------------------------------------------------------
// Set 0, binding 0 — CameraData UBO (matches vk_frame.h exactly).
// std140: each mat4 = 64 bytes, vec3 padded to 16.
// -----------------------------------------------------------------------
layout(set = 0, binding = 0) uniform CameraData {
    mat4 view;
    mat4 proj;
    mat4 view_proj;   // pre-multiplied; use this to save a multiply per vertex
    vec3 camera_pos;
    float _pad;
} camera;

// -----------------------------------------------------------------------
// Push constants — model matrix at offset 0 (vertex stage).
// Fragment tint follows at offset 64; not visible here.
// -----------------------------------------------------------------------
layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

// -----------------------------------------------------------------------
// Outputs
// -----------------------------------------------------------------------
layout(location = 0) out vec3 frag_world_pos;
layout(location = 1) out vec3 frag_world_normal;
layout(location = 2) out vec3 frag_color;
layout(location = 3) out vec2 frag_uv;

void main()
{
    vec4 world_pos    = push.model * vec4(in_position, 1.0);
    frag_world_pos    = world_pos.xyz;

    // Inverse-transpose handles non-uniform scaling correctly.
    frag_world_normal = normalize(mat3(transpose(inverse(push.model))) * in_normal);

    frag_color = in_color;
    frag_uv    = in_uv;

    gl_Position = camera.view_proj * world_pos;
}
