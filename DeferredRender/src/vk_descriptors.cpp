#include "vk_descriptors.h"

DescriptorAllocator create_descriptor_allocator(const VkContext& ctx)
{
	std::vector<VkDescriptorPoolSize> pool_sizes = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_FRAMES_IN_FLIGHT },
	};
	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.poolSizeCount = static_cast<u32>(pool_sizes.size());
	pool_info.pPoolSizes = pool_sizes.data();
	pool_info.maxSets = MAX_FRAMES_IN_FLIGHT;
	DescriptorAllocator allocator;
	VK_CHECK(vkCreateDescriptorPool(ctx.device, &pool_info, nullptr, &allocator.pool));
	return allocator;
}

void destroy_descriptor_allocator(const VkContext& ctx, DescriptorAllocator& allocator)
{
	vkDestroyDescriptorPool(ctx.device, allocator.pool, nullptr);
	allocator.pool = VK_NULL_HANDLE;
}

VkDescriptorSetLayout create_ubo_set_layout(const VkContext& ctx)
{
	VkDescriptorSetLayoutBinding ubo_binding = {};
	ubo_binding.binding = 0;
	ubo_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo_binding.descriptorCount = 1;
	ubo_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo layout_info = {};
	layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount = 1;
	layout_info.pBindings = &ubo_binding;

	VkDescriptorSetLayout layout;
	VK_CHECK(vkCreateDescriptorSetLayout(ctx.device, &layout_info, nullptr, &layout));
	return layout;
}

std::vector<VkDescriptorSet> create_ubo_descriptor_sets(
	const VkContext& ctx,
	VkDescriptorSetLayout  layout,
	VkDescriptorPool       pool,
	const FrameUBO         ubos[MAX_FRAMES_IN_FLIGHT])
{
    // Allocate one set per frame-in-flight - all from the same layout
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, layout);

    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = pool;
    alloc_info.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    alloc_info.pSetLayouts = layouts.data();

    std::vector<VkDescriptorSet> sets(MAX_FRAMES_IN_FLIGHT);
    VK_CHECK(vkAllocateDescriptorSets(ctx.device, &alloc_info, sets.data()));

    // Write the UBO binding into each set - this wires the buffer to binding 0
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorBufferInfo buf_info = {};
        buf_info.buffer = ubos[i].buffer.buffer;
        buf_info.offset = 0;
        buf_info.range = sizeof(UniformData); // or VK_WHOLE_SIZE

        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = sets[i];
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &buf_info;

        vkUpdateDescriptorSets(ctx.device, 1, &write, 0, nullptr);
    }

    LOG_DEBUG_TO("render", "Descriptor sets written for {} frames", MAX_FRAMES_IN_FLIGHT);
    return sets;
}