#include "vk_material.h"
#include <array>

VkDescriptorSetLayout vk_create_material_set_layout(VkDevice device)
{
    // Binding 0: combined image sampler (albedo)
    // Phase 4: add bindings 1 (normal), 2 (roughness/metallic), 3 (emissive)
    VkDescriptorSetLayoutBinding albedo_binding = {};
    albedo_binding.binding = 0;
    albedo_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    albedo_binding.descriptorCount = 1;
    albedo_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    albedo_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 1;
    layout_info.pBindings = &albedo_binding;

    VkDescriptorSetLayout layout;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &layout));
    return layout;
}

VkDescriptorPool vk_create_material_descriptor_pool(VkDevice device, u32 max_materials)
{
    // Each material needs MAX_FRAMES_IN_FLIGHT sets, each with 1 sampler binding
    VkDescriptorPoolSize pool_size = {};
    pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_size.descriptorCount = max_materials * MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = max_materials * MAX_FRAMES_IN_FLIGHT;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;

    VkDescriptorPool pool;
    VK_CHECK(vkCreateDescriptorPool(device, &pool_info, nullptr, &pool));
    LOG_DEBUG_TO("render", "Material descriptor pool created (max {} materials)",
        max_materials);
    return pool;
}

void vk_write_material_descriptors(
    VkDevice              device,
    VkDescriptorPool      pool,
    VkDescriptorSetLayout layout,
    Material& mat)
{
    // --- allocate one set per frame-in-flight ---
    std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts;
    layouts.fill(layout);

    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = pool;
    alloc_info.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    alloc_info.pSetLayouts = layouts.data();

    VK_CHECK(vkAllocateDescriptorSets(device, &alloc_info, mat.descriptor_sets));

    // --- write albedo image into each set ---
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        VkDescriptorImageInfo image_info = {};
        image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_info.imageView = mat.albedo->image.view;
        image_info.sampler = mat.albedo->sampler;

        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = mat.descriptor_sets[i];
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &image_info;

        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }

    LOG_DEBUG_TO("render", "Material descriptor sets written (type={})",
        (int)mat.type);
}