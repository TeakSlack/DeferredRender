#include "vk_uniforms.h"

FrameUBO create_frame_ubo(const VkContext& ctx)
{
	FrameUBO ubo;

	ubo.buffer = vk_create_buffer(
		ctx.allocator,
		sizeof(UniformData),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_MEMORY_USAGE_AUTO,
		VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT);

	ubo.mapped = ubo.buffer.info.pMappedData;
	return ubo;
}

void destroy_frame_ubo(const VkContext& ctx, FrameUBO& ubo)
{
	vk_destroy_buffer(ctx.allocator, ubo.buffer);
	ubo.mapped = nullptr;
}