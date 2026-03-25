#include "vk_frame.h"
#include <cstring>

VkDescriptorSetLayout vk_create_frame_set_layout(VkDevice device)
{
    // Binding 0: camera UBO - vertex + fragment both read it (proj for vert,
    // camera_pos for fragment PBR lighting)
    VkDescriptorSetLayoutBinding ubo_binding = {};
    ubo_binding.binding = 0;
    ubo_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_binding.descriptorCount = 1;
    ubo_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
        VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 1;
    layout_info.pBindings = &ubo_binding;

    VkDescriptorSetLayout layout;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &layout));
    return layout;
}

VkDescriptorPool vk_create_frame_descriptor_pool(VkDevice device)
{
    VkDescriptorPoolSize pool_size = {};
    pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_size.descriptorCount = MAX_FRAMES_IN_FLIGHT;

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.maxSets = MAX_FRAMES_IN_FLIGHT;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;

    VkDescriptorPool pool;
    VK_CHECK(vkCreateDescriptorPool(device, &pool_info, nullptr, &pool));
    return pool;
}

void vk_create_frame_resources(
    VmaAllocator          allocator,
    VkDevice              device,
    VkDescriptorPool      pool,
    VkDescriptorSetLayout layout,
    FrameResources        out_frames[MAX_FRAMES_IN_FLIGHT])
{
    // Allocate all descriptor sets in one call
    VkDescriptorSetLayout layouts[MAX_FRAMES_IN_FLIGHT];
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        layouts[i] = layout;

    VkDescriptorSet sets[MAX_FRAMES_IN_FLIGHT];
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = pool;
    alloc_info.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
    alloc_info.pSetLayouts = layouts;
    VK_CHECK(vkAllocateDescriptorSets(device, &alloc_info, sets));

    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        // Persistently mapped UBO - written each frame with memcpy
        out_frames[i].camera_ubo = vk_create_buffer(
            allocator,
            sizeof(CameraData),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_MEMORY_USAGE_AUTO,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT);

        out_frames[i].camera_mapped =
            reinterpret_cast<CameraData*>(out_frames[i].camera_ubo.info.pMappedData);
        out_frames[i].frame_set = sets[i];

        // Wire the UBO into the descriptor set
        VkDescriptorBufferInfo buf_info = {};
        buf_info.buffer = out_frames[i].camera_ubo.buffer;
        buf_info.offset = 0;
        buf_info.range = sizeof(CameraData);

        VkWriteDescriptorSet write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = sets[i];
        write.dstBinding = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &buf_info;

        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
    }

    LOG_DEBUG_TO("render", "Frame resources created for {} frames-in-flight",
        MAX_FRAMES_IN_FLIGHT);
}

void vk_destroy_frame_resources(
    VmaAllocator   allocator,
    FrameResources frames[MAX_FRAMES_IN_FLIGHT])
{
    for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vk_destroy_buffer(allocator, frames[i].camera_ubo);
        frames[i].camera_mapped = nullptr;
        // Descriptor sets are freed when the pool is destroyed - no individual free needed
        frames[i].frame_set = VK_NULL_HANDLE;
    }
}