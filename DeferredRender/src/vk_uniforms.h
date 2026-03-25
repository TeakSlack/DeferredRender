#ifndef VK_UNIFORMS_H
#define VK_UNIFORMS_H

#include "vk_types.h"
#include "vk_buffer.h"
#include <glm/glm.hpp>

struct alignas(16) UniformData
{
	glm::mat4 model;
	glm::mat4 view;
	glm::mat4 proj;
};

// One UBO per frame in flight
struct FrameUBO
{
	AllocatedBuffer buffer;
	void* mapped = nullptr;

	void write(const UniformData& ubo)
	{
		memcpy(mapped, &ubo, sizeof(ubo));
	}
};

FrameUBO create_frame_ubo(const VkContext& ctx);
void destroy_frame_ubo(const VkContext& ctx, FrameUBO& ubo);

#endif // VK_UNIFORMS_H