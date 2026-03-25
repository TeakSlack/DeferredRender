#ifndef VK_MATERIAL_H
#define VK_MATERIAL_H

#include "vk_types.h"
#include "vk_texture.h"

// -------------------------------------------------------------------------
// MaterialType - drives pipeline selection.
// Each type may require a different VkPipeline (e.g. masked needs
// alpha-test discard in the fragment shader, transparent needs blending).
// -------------------------------------------------------------------------
enum class MaterialType : u8 {
    PBROpaque = 0,
    PBRMasked = 1,  // alpha-tested: discard fragments below threshold
    Unlit = 2,
};

// -------------------------------------------------------------------------
// MaterialPushConstants - per-draw scalars pushed via vkCmdPushConstants.
// Keeps small per-object data out of descriptor sets entirely.
// sizeof == 16 bytes - fits in the minimum guaranteed push constant budget.
// -------------------------------------------------------------------------
struct MaterialPushConstants {
    float albedo_tint[4] = { 1, 1, 1, 1 }; // multiplied with albedo texture
};

// -------------------------------------------------------------------------
// Material - owns its descriptor sets (one per frame-in-flight).
// Textures are referenced by pointer/handle; the TextureCache owns them.
// -------------------------------------------------------------------------
struct Material {
    MaterialType type = MaterialType::PBROpaque;

    // Albedo texture - always present (falls back to 1×1 white)
    const Texture* albedo = nullptr;

    // Per-frame descriptor sets (set = 1 in the shader)
    // Updated once at creation; only needs rebuilding on hot-reload.
    VkDescriptorSet descriptor_sets[MAX_FRAMES_IN_FLIGHT] = {};

    // Push constants applied before each draw call
    MaterialPushConstants push_constants;
};

// -------------------------------------------------------------------------
// Descriptor layout - one for the entire material system.
// Set 0 = per-frame globals (camera UBO)
// Set 1 = per-material (albedo sampler at binding 0)
//
// This layout is shared by all material types in Phase 3.
// Phase 4 expands binding 0..N for PBR maps.
// -------------------------------------------------------------------------
VkDescriptorSetLayout vk_create_material_set_layout(VkDevice device);

// -------------------------------------------------------------------------
// Descriptor pool sized for (MAX_FRAMES_IN_FLIGHT × max_materials) sets.
// Create once per application lifetime.
// -------------------------------------------------------------------------
VkDescriptorPool vk_create_material_descriptor_pool(
    VkDevice device, u32 max_materials);

// -------------------------------------------------------------------------
// Allocate and write descriptor sets for one material.
// Writes the albedo image view + sampler into binding 0 of set 1.
// Call once after creating the material; result stored in mat.descriptor_sets.
// -------------------------------------------------------------------------
void vk_write_material_descriptors(
    VkDevice              device,
    VkDescriptorPool      pool,
    VkDescriptorSetLayout layout,
    Material& mat);

#endif