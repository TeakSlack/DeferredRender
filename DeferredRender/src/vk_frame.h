#ifndef VK_FRAME_H
#define VK_FRAME_H

#include "vk_types.h"
#include "vk_buffer.h"
#include "vk_context.h"

#include <glm/glm.hpp>

// -------------------------------------------------------------------------
// CameraData - the per-frame uniform buffer written to set 0, binding 0.
// Layout matches the GLSL uniform block in mesh.vert exactly.
// std140 alignment: each mat4 is 64 bytes, vec3 padded to 16.
// -------------------------------------------------------------------------
struct alignas(16) CameraData {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 view_proj;        // pre-multiplied for the vertex shader
    glm::vec3 camera_pos;
    float     _pad = 0.0f;      // explicit padding to vec4 boundary
};

// -------------------------------------------------------------------------
// FrameResources - all per-frame-in-flight GPU objects.
// One instance per slot in MAX_FRAMES_IN_FLIGHT.
// -------------------------------------------------------------------------
struct FrameResources {
    // Camera UBO - persistently mapped, written every frame
    AllocatedBuffer camera_ubo;
    CameraData* camera_mapped = nullptr; // points into camera_ubo

    // Descriptor set for set 0 (camera UBO at binding 0)
    VkDescriptorSet frame_set = VK_NULL_HANDLE;
};

// -------------------------------------------------------------------------
// Descriptor set layout for set 0.
// Called once at startup; result shared across all pipelines.
// -------------------------------------------------------------------------
VkDescriptorSetLayout vk_create_frame_set_layout(VkDevice device);

// -------------------------------------------------------------------------
// Descriptor pool for frame sets.
// Sized for MAX_FRAMES_IN_FLIGHT sets each containing one UBO binding.
// -------------------------------------------------------------------------
VkDescriptorPool vk_create_frame_descriptor_pool(VkDevice device);

// -------------------------------------------------------------------------
// Create all per-frame resources and write the descriptor sets.
// out_frames must point to an array of MAX_FRAMES_IN_FLIGHT FrameResources.
// -------------------------------------------------------------------------
void vk_create_frame_resources(
    VmaAllocator          allocator,
    VkDevice              device,
    VkDescriptorPool      pool,
    VkDescriptorSetLayout layout,
    FrameResources        out_frames[MAX_FRAMES_IN_FLIGHT]);

void vk_destroy_frame_resources(
    VmaAllocator   allocator,
    FrameResources frames[MAX_FRAMES_IN_FLIGHT]);

#endif // VK_FRAME_H