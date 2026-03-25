#ifndef VK_MESH_H
#define VK_MESH_H

#include "vk_types.h"
#include "vk_buffer.h"
#include "pipeline_builder.h"
#include <array>
#include <vector>
#include <glm/glm.hpp>

// -------------------------------------------------------------------------
// Vertex - the per-vertex data layout bound to binding 0.
// Matches the attribute declarations in the vertex shader exactly.
// -------------------------------------------------------------------------
struct Vertex {
	glm::vec3 position;  // location = 0
	glm::vec3 color;     // location = 1
	glm::vec2 uv;        // location = 2


    // These tell the pipeline how to interpret a tightly-packed array of Vertex
    static consteval VkVertexInputBindingDescription get_binding_description() {
        VkVertexInputBindingDescription desc = {};
        desc.binding = 0;
        desc.stride = sizeof(Vertex);
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // advance per vertex, not per instance
        return desc;
    }

    static consteval std::array<VkVertexInputAttributeDescription, 3> get_attribute_descriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attrs = {};

        attrs[0].binding = 0;
        attrs[0].location = 0;
        attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[0].offset = offsetof(Vertex, position);

        attrs[1].binding = 0;
        attrs[1].location = 1;
        attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[1].offset = offsetof(Vertex, color);

        attrs[2].binding = 0;
        attrs[2].location = 2;
        attrs[2].format = VK_FORMAT_R32G32_SFLOAT;
        attrs[2].offset = offsetof(Vertex, uv);

        return attrs;
    }
};

// -------------------------------------------------------------------------
// Mesh - owns its GPU-resident vertex and index buffers.
// -------------------------------------------------------------------------
struct Mesh {
    AllocatedBuffer vertex_buffer;
    AllocatedBuffer index_buffer;
    u32             index_count = 0;
};

// -------------------------------------------------------------------------
// Cube geometry - 24 unique vertices (4 per face, so UVs are per-face)
// and 36 indices (6 per face × 6 faces).
// -------------------------------------------------------------------------
void get_cube_geometry(std::vector<Vertex>& out_vertices,
    std::vector<u32>& out_indices);

// Upload geometry to DEVICE_LOCAL buffers via staging.
// The one-shot copy command uses the graphics queue.
Mesh create_mesh(const VkContext& ctx,
    const std::vector<Vertex>& vertices,
    const std::vector<u32>& indices);

void destroy_mesh(VmaAllocator allocator, Mesh& mesh);

#endif // VK_MESH_H