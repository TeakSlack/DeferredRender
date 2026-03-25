#ifndef VK_DESCRIPTORS_H
#define VK_DESCRIPTORS_H

#include "vk_types.h"
#include "vk_context.h"
#include "vk_uniforms.h"
#include <vector>

struct DescriptorAllocator
{
	VkDescriptorPool pool = VK_NULL_HANDLE;
};

DescriptorAllocator create_descriptor_allocator(const VkContext& ctx);
void destroy_descriptor_allocator(const VkContext& ctx, DescriptorAllocator& allocator);

VkDescriptorSetLayout create_ubo_set_layout(const VkContext& ctx);

std::vector<VkDescriptorSet> create_ubo_descriptor_sets(
	const VkContext&	   ctx,
	VkDescriptorSetLayout  layout,
	VkDescriptorPool       pool,
	const FrameUBO         ubos[MAX_FRAMES_IN_FLIGHT]);

#endif // VK_DESCRIPTORS_H