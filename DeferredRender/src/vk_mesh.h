#ifndef VK_MESH_H
#define VK_MESH_H

#include "vk_types.h"
#include "vk_buffer.h"
#include "vk_context.h"

#include <glm/glm.hpp>
#include <array>
#include <vector>

// -------------------------------------------------------------------------
// Vertex - the per-vertex data layout bound to binding 0.
// Includes normals now so Phase 4 (G-buffer) can write them without
// changing the vertex format or pipeline vertex input state.
// -------------------------------------------------------------------------
struct Vertex {
    glm::vec3 position;  // location = 0
    glm::vec3 normal;    // location = 1
    glm::vec3 color;     // location = 2
    glm::vec2 uv;        // location = 3

    static VkVertexInputBindingDescription binding_desc()
    {
        VkVertexInputBindingDescription desc = {};
        desc.binding = 0;
        desc.stride = sizeof(Vertex);
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return desc;
    }

    static std::array<VkVertexInputAttributeDescription, 4> attribute_descs()
    {
        std::array<VkVertexInputAttributeDescription, 4> attrs = {};

        attrs[0].binding = 0;
        attrs[0].location = 0;
        attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[0].offset = offsetof(Vertex, position);

        attrs[1].binding = 0;
        attrs[1].location = 1;
        attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[1].offset = offsetof(Vertex, normal);

        attrs[2].binding = 0;
        attrs[2].location = 2;
        attrs[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[2].offset = offsetof(Vertex, color);

        attrs[3].binding = 0;
        attrs[3].location = 3;
        attrs[3].format = VK_FORMAT_R32G32_SFLOAT;
        attrs[3].offset = offsetof(Vertex, uv);

        return attrs;
    }
};

// -------------------------------------------------------------------------
// Mesh - owns its GPU-resident vertex and index buffers.
// The mesh does NOT own a material; that lives on the RenderObject.
// -------------------------------------------------------------------------
struct Mesh {
    AllocatedBuffer vertex_buffer;
    AllocatedBuffer index_buffer;
    u32             index_count = 0;
    u32             vertex_count = 0;
};

// Upload geometry to DEVICE_LOCAL buffers via one-shot staging transfer.
Mesh vk_create_mesh(
    const VkContext& ctx,
    VmaAllocator              allocator,
    const std::vector<Vertex>& vertices,
    const std::vector<u32>& indices);

void vk_destroy_mesh(VmaAllocator allocator, Mesh& mesh);

// -------------------------------------------------------------------------
// Cube geometry helper - 24 vertices (4 per face for independent UVs),
// 36 indices (6 per face).
// -------------------------------------------------------------------------
void get_cube_geometry(std::vector<Vertex>& out_vertices,
    std::vector<u32>& out_indices);

#endif // VK_MESH_H