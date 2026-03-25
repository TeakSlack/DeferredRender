#version 450

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_color;
layout (location = 2) in vec2 in_uv;

layout (set = 0, binding = 0) uniform UniformData {
	mat4 model;
	mat4 view;
	mat4 proj;
} ubo;

layout (location = 0) out vec3 frag_color;
layout(location = 1) out vec2 frag_uv;

void main() {
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(in_position, 1.0);
	frag_color = in_color;
	frag_uv = in_uv;
}