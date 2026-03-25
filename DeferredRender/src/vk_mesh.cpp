#include "vk_mesh.h"

Mesh vk_create_mesh(
    const VkContext& ctx,
    VmaAllocator               allocator,
    const std::vector<Vertex>& vertices,
    const std::vector<u32>& indices)
{
    const VkDeviceSize vb_size = vertices.size() * sizeof(Vertex);
    const VkDeviceSize ib_size = indices.size() * sizeof(u32);

    AllocatedBuffer vert_staging = vk_create_staging_buffer(
        allocator, vertices.data(), vb_size);
    AllocatedBuffer idx_staging = vk_create_staging_buffer(
        allocator, indices.data(), ib_size);

    Mesh mesh;
    mesh.vertex_count = (u32)vertices.size();
    mesh.index_count = (u32)indices.size();

    mesh.vertex_buffer = vk_create_buffer(
        allocator, vb_size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

    mesh.index_buffer = vk_create_buffer(
        allocator, ib_size,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);

    vk_immediate_submit(ctx, [&](VkCommandBuffer cmd) {
        VkBufferCopy region = {};

        region.size = vb_size;
        vkCmdCopyBuffer(cmd, vert_staging.buffer, mesh.vertex_buffer.buffer, 1, &region);

        region.size = ib_size;
        vkCmdCopyBuffer(cmd, idx_staging.buffer, mesh.index_buffer.buffer, 1, &region);
        });

    vk_destroy_buffer(allocator, vert_staging);
    vk_destroy_buffer(allocator, idx_staging);

    LOG_INFO_TO("render", "Mesh uploaded: {} vertices, {} indices",
        vertices.size(), indices.size());
    return mesh;
}

void vk_destroy_mesh(VmaAllocator allocator, Mesh& mesh)
{
    vk_destroy_buffer(allocator, mesh.vertex_buffer);
    vk_destroy_buffer(allocator, mesh.index_buffer);
}

// -------------------------------------------------------------------------
// Cube geometry
// -------------------------------------------------------------------------
void get_cube_geometry(std::vector<Vertex>& verts, std::vector<u32>& idx)
{
    verts.clear();
    idx.clear();

    auto quad = [&](
        glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3,
        glm::vec3 normal, glm::vec3 color)
        {
            u32 base = (u32)verts.size();
            verts.push_back({ p0, normal, color, { 0, 0 } });
            verts.push_back({ p1, normal, color, { 1, 0 } });
            verts.push_back({ p2, normal, color, { 1, 1 } });
            verts.push_back({ p3, normal, color, { 0, 1 } });
            idx.insert(idx.end(), {
                base + 0, base + 1, base + 2,
                base + 2, base + 3, base + 0
                });
        };

    // +Z front   (red)
    quad({ -0.5f,-0.5f, 0.5f }, { 0.5f,-0.5f, 0.5f },
        { 0.5f, 0.5f, 0.5f }, { -0.5f, 0.5f, 0.5f },
        { 0, 0, 1 }, { 1,0,0 });
    // -Z back    (green)
    quad({ 0.5f,-0.5f,-0.5f }, { -0.5f,-0.5f,-0.5f },
        { -0.5f, 0.5f,-0.5f }, { 0.5f, 0.5f,-0.5f },
        { 0, 0,-1 }, { 0,1,0 });
    // +X right   (blue)
    quad({ 0.5f,-0.5f, 0.5f }, { 0.5f,-0.5f,-0.5f },
        { 0.5f, 0.5f,-0.5f }, { 0.5f, 0.5f, 0.5f },
        { 1, 0, 0 }, { 0,0,1 });
    // -X left    (yellow)
    quad({ -0.5f,-0.5f,-0.5f }, { -0.5f,-0.5f, 0.5f },
        { -0.5f, 0.5f, 0.5f }, { -0.5f, 0.5f,-0.5f },
        { -1, 0, 0 }, { 1,1,0 });
    // +Y top     (cyan)
    quad({ -0.5f, 0.5f, 0.5f }, { 0.5f, 0.5f, 0.5f },
        { 0.5f, 0.5f,-0.5f }, { -0.5f, 0.5f,-0.5f },
        { 0, 1, 0 }, { 0,1,1 });
    // -Y bottom  (magenta)
    quad({ -0.5f,-0.5f,-0.5f }, { 0.5f,-0.5f,-0.5f },
        { 0.5f,-0.5f, 0.5f }, { -0.5f,-0.5f, 0.5f },
        { 0,-1, 0 }, { 1,0,1 });
}