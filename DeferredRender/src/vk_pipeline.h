#ifndef VK_PIPELINE_H
#define VK_PIPELINE_H

#include "vk_types.h"
#include "vk_mesh.h"
#include "vk_material.h"

// -------------------------------------------------------------------------
// PipelineLayout - owns the VkPipelineLayout describing:
//   Set 0: per-frame globals (camera UBO)            [vk_frame.h]
//   Set 1: per-material (albedo sampler)             [vk_material.h]
//   Push constants:
//     offset 0  size 64  VK_SHADER_STAGE_VERTEX_BIT   - mat4 model
//     offset 64 size 16  VK_SHADER_STAGE_FRAGMENT_BIT - vec4 albedo_tint
// -------------------------------------------------------------------------
VkPipelineLayout vk_create_pipeline_layout(
    VkDevice              device,
    VkDescriptorSetLayout frame_set_layout,    // set 0
    VkDescriptorSetLayout material_set_layout); // set 1

// -------------------------------------------------------------------------
// Graphics pipeline for opaque PBR geometry.
// Built once; compatible with any render pass that has the same attachment
// formats (color R8G8B8A8_SRGB + depth D32_SFLOAT).
// -------------------------------------------------------------------------
struct PipelineCreateInfo {
    VkRenderPass     render_pass;
    VkPipelineLayout layout;
    VkShaderModule   vert_module;
    VkShaderModule   frag_module;
    VkExtent2D       viewport_extent;
    MaterialType     material_type; // drives cull mode, blend state, etc.
};

VkPipeline vk_create_graphics_pipeline(VkDevice device, const PipelineCreateInfo& info);

// -------------------------------------------------------------------------
// Shader module helpers
// -------------------------------------------------------------------------
VkShaderModule vk_load_shader(VkDevice device, const char* spv_path);
void           vk_destroy_shader(VkDevice device, VkShaderModule mod);

#endif // VK_PIPELINE_H