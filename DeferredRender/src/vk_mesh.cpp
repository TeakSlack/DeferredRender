#include "vk_mesh.h"
#include "vk_context.h"
#include <vector>

static void immediate_submit(const VkContext& ctx, std::function<void(VkCommandBuffer)> fn)
{
	VkCommandPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	pool_info.queueFamilyIndex = ctx.queue_families.graphics;

	VkCommandPool cmd_pool;
	VK_CHECK(vkCreateCommandPool(ctx.device, &pool_info, nullptr, &cmd_pool));

	VkCommandBufferAllocateInfo alloc_info = {};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandPool = cmd_pool;
	alloc_info.commandBufferCount = 1;

	VkCommandBuffer cmd;
	VK_CHECK(vkAllocateCommandBuffers(ctx.device, &alloc_info, &cmd));

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));	
	fn(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd;

	VK_CHECK(vkQueueSubmit(ctx.graphics_queue, 1, &submit_info, VK_NULL_HANDLE));
	VK_CHECK(vkQueueWaitIdle(ctx.graphics_queue));

	vkDestroyCommandPool(ctx.device, cmd_pool, nullptr);
}

Mesh create_mesh(const VkContext& ctx, const std::vector<Vertex>& vertices, const std::vector<u32>& indices)
{
	Mesh mesh = {};
	mesh.index_count = static_cast<u32>(indices.size());

	AllocatedBuffer vertex_buffer = vk_create_staged_buffer(
		ctx,
		ctx.allocator,
		vertices.data(),
		sizeof(vertices[0]) * vertices.size(),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

	AllocatedBuffer index_buffer = vk_create_staged_buffer(
		ctx,
		ctx.allocator,
		indices.data(),
		sizeof(indices[0]) * indices.size(),
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

	mesh.vertex_buffer = vertex_buffer;
	mesh.index_buffer = index_buffer;

	LOG_INFO_TO("render", "Mesh created with {} vertices and {} indices",
		vertices.size(), indices.size());

	return mesh;
}

void destroy_mesh(VmaAllocator allocator, Mesh& mesh)
{
	vmaDestroyBuffer(allocator, mesh.vertex_buffer.buffer, mesh.vertex_buffer.allocation);
	vmaDestroyBuffer(allocator, mesh.index_buffer.buffer, mesh.index_buffer.allocation);
	mesh = {};
}

void get_cube_geometry(std::vector<Vertex>& verts, std::vector<u32>& idx)
{
	// Helper: push 4 vertices (a quad) and 2 triangles (6 indices)
	u32 base = 0;
	auto quad = [&](Vertex v0, Vertex v1, Vertex v2, Vertex v3) {
		base = (u32)verts.size();
		verts.insert(verts.end(), { v0, v1, v2, v3 });
		// Two CCW triangles from the quad
		idx.insert(idx.end(), {
			base + 0, base + 1, base + 2,
			base + 2, base + 3, base + 0
			});
		};

	// +Z face (front) - red
	quad({ {-0.5f,-0.5f, 0.5f}, {1,0,0}, {0,0} },
		{ { 0.5f,-0.5f, 0.5f}, {1,0,0}, {1,0} },
		{ { 0.5f, 0.5f, 0.5f}, {1,0,0}, {1,1} },
		{ {-0.5f, 0.5f, 0.5f}, {1,0,0}, {0,1} });

	// -Z face (back) - green
	quad({ { 0.5f,-0.5f,-0.5f}, {0,1,0}, {0,0} },
		{ {-0.5f,-0.5f,-0.5f}, {0,1,0}, {1,0} },
		{ {-0.5f, 0.5f,-0.5f}, {0,1,0}, {1,1} },
		{ { 0.5f, 0.5f,-0.5f}, {0,1,0}, {0,1} });

	// +X face (right) - blue
	quad({ { 0.5f,-0.5f, 0.5f}, {0,0,1}, {0,0} },
		{ { 0.5f,-0.5f,-0.5f}, {0,0,1}, {1,0} },
		{ { 0.5f, 0.5f,-0.5f}, {0,0,1}, {1,1} },
		{ { 0.5f, 0.5f, 0.5f}, {0,0,1}, {0,1} });

	// -X face (left) - yellow
	quad({ {-0.5f,-0.5f,-0.5f}, {1,1,0}, {0,0} },
		{ {-0.5f,-0.5f, 0.5f}, {1,1,0}, {1,0} },
		{ {-0.5f, 0.5f, 0.5f}, {1,1,0}, {1,1} },
		{ {-0.5f, 0.5f,-0.5f}, {1,1,0}, {0,1} });

	// +Y face (top) - cyan
	quad({ {-0.5f, 0.5f, 0.5f}, {0,1,1}, {0,0} },
		{ { 0.5f, 0.5f, 0.5f}, {0,1,1}, {1,0} },
		{ { 0.5f, 0.5f,-0.5f}, {0,1,1}, {1,1} },
		{ {-0.5f, 0.5f,-0.5f}, {0,1,1}, {0,1} });

	// -Y face (bottom) - magenta
	quad({ {-0.5f,-0.5f,-0.5f}, {1,0,1}, {0,0} },
		{ { 0.5f,-0.5f,-0.5f}, {1,0,1}, {1,0} },
		{ { 0.5f,-0.5f, 0.5f}, {1,0,1}, {1,1} },
		{ {-0.5f,-0.5f, 0.5f}, {1,0,1}, {0,1} });
}