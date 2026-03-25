#version 450

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;   // reserved for G-buffer pass
layout (location = 2) in vec3 in_color;
layout (location = 3) in vec2 in_uv;

layout (set = 0, binding = 0) uniform CameraData {
    mat4 view;
    mat4 proj;
    mat4 view_proj;
    vec3 camera_pos;
    float _pad;
} camera;

layout (push_constant) uniform PushConstants {
    mat4 model;
} pc;

layout (location = 0) out vec3 frag_color;
layout (location = 1) out vec2 frag_uv;

void main() {
    gl_Position = camera.view_proj * pc.model * vec4(in_position, 1.0);
    frag_color  = in_color;
    frag_uv     = in_uv;
}
