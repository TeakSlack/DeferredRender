#include "vk_buffer.h"
#include "logger.h"

AllocatedBuffer vk_create_buffer(
    VmaAllocator             allocator,
    VkDeviceSize             size,
    VkBufferUsageFlags       usage_flags,
    VmaMemoryUsage           memory_usage,
    VmaAllocationCreateFlags alloc_flags)
{
    VkBufferCreateInfo buf_info = {};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.size = size;
    buf_info.usage = usage_flags;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_create_info = {};
    alloc_create_info.usage = memory_usage;
    alloc_create_info.flags = alloc_flags;

    AllocatedBuffer result;
    VK_CHECK(vmaCreateBuffer(allocator, &buf_info, &alloc_create_info,
        &result.buffer, &result.allocation, &result.info));

    LOG_DEBUG_TO("render", "Buffer created: {} bytes, usage={:#x}",
        (u64)size, (u32)usage_flags);
    return result;
}

void vk_destroy_buffer(VmaAllocator allocator, AllocatedBuffer& buf) {
    vmaDestroyBuffer(allocator, buf.buffer, buf.allocation);
    buf.buffer = VK_NULL_HANDLE;
    buf.allocation = VK_NULL_HANDLE;
}

void vk_copy_buffer(const VkContext& ctx, VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = ctx.transfer_cmd_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer cmd;
    VK_CHECK(vkAllocateCommandBuffers(ctx.device, &alloc_info, &cmd));

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(cmd, &begin_info));

    VkBufferCopy copy_region = {};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = size;
    vkCmdCopyBuffer(cmd, src, dst, 1, &copy_region);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence;
    VK_CHECK(vkCreateFence(ctx.device, &fence_info, nullptr, &fence));

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd;
    VK_CHECK(vkQueueSubmit(ctx.transfer_queue, 1, &submit_info, fence));

    VK_CHECK(vkWaitForFences(ctx.device, 1, &fence, VK_TRUE, UINT64_MAX));
    vkFreeCommandBuffers(ctx.device, ctx.transfer_cmd_pool, 1, &cmd);
	vkDestroyFence(ctx.device, fence, nullptr);
}

AllocatedBuffer vk_create_staging_buffer(
    VmaAllocator allocator, const void* data, VkDeviceSize size)
{
    // HOST_ACCESS_SEQUENTIAL_WRITE tells VMA this is a CPU->GPU upload path.
    // VMA will pick a HOST_VISIBLE | HOST_COHERENT heap automatically.
    // MAPPED_BIT means VMA maps it for us and stores the pointer in info.pMappedData.
    AllocatedBuffer staging = vk_create_buffer(
        allocator, size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
        VMA_ALLOCATION_CREATE_MAPPED_BIT);

    memcpy(staging.info.pMappedData, data, (size_t)size);
    // No explicit flush needed - HOST_COHERENT memory is always visible to GPU
    return staging;
}

AllocatedBuffer vk_create_staged_buffer(
    const VkContext &ctx,
    VmaAllocator allocator,
    const void* data,
    VkDeviceSize size,
    VkBufferUsageFlags usage_flags)
{
	AllocatedBuffer staging = vk_create_staging_buffer(allocator, data, size);
	AllocatedBuffer result = vk_create_buffer(
		allocator, size,
		usage_flags | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	vk_copy_buffer(ctx, staging.buffer, result.buffer, size);
	vk_destroy_buffer(allocator, staging);
	return result;
}

AllocatedImage vk_create_image(
    VmaAllocator       allocator,
    VkDevice           device,
    VkFormat           format,
    VkExtent2D         extent,
    VkImageUsageFlags  usage,
    VkImageAspectFlags aspect,
    u32                mip_levels)
{
    VkImageCreateInfo img_info = {};
    img_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    img_info.imageType = VK_IMAGE_TYPE_2D;
    img_info.format = format;
    img_info.extent = { extent.width, extent.height, 1 };
    img_info.mipLevels = mip_levels;
    img_info.arrayLayers = 1;
    img_info.samples = VK_SAMPLE_COUNT_1_BIT;
    img_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    img_info.usage = usage;
    img_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    // DEVICE_LOCAL is always correct for GPU-resident render targets/textures
    VmaAllocationCreateInfo alloc_create_info = {};
    alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

    AllocatedImage result;
    result.format = format;
    result.extent = extent;
    VK_CHECK(vmaCreateImage(allocator, &img_info, &alloc_create_info,
        &result.image, &result.allocation, &result.info));

    // Create the image view immediately - it's almost always needed alongside
    VkImageViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = result.image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = aspect;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = mip_levels;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;
    VK_CHECK(vkCreateImageView(device, &view_info, nullptr, &result.view));

    LOG_DEBUG_TO("render", "Image created: {}x{} fmt={} usage={:#x}",
        extent.width, extent.height, (int)format, (u32)usage);
    return result;
}

void vk_transition_image_layout(
    VkCommandBuffer cmd,
    VkImage         image,
    VkImageLayout   old_layout,
    VkImageLayout   new_layout,
    u32             mip_levels,
    u32             layer_count)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask =
        (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
            old_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        ? VK_IMAGE_ASPECT_DEPTH_BIT
        : VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mip_levels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layer_count;

    VkPipelineStageFlags src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags dst_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    // Determine access masks and pipeline stages from the layout pair
    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        src_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dst_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dst_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    else
    {
        LOG_WARN_TO("render", "vk_transition_image_layout: unhandled layout pair "
            "{} -> {}", (int)old_layout, (int)new_layout);
        // Fallback - safe but not optimal
        barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
        src_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        dst_stage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }

    vkCmdPipelineBarrier(cmd,
        src_stage, dst_stage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier);
}


void vk_destroy_image(VmaAllocator allocator, VkDevice device, AllocatedImage& img) {
    vkDestroyImageView(device, img.view, nullptr);
    vmaDestroyImage(allocator, img.image, img.allocation);
    img.image = VK_NULL_HANDLE;
    img.view = VK_NULL_HANDLE;
    img.allocation = VK_NULL_HANDLE;
}

AllocatedImage create_depth_image(const VkContext& ctx,
    VkExtent2D extent)
{
    return vk_create_image(
        ctx.allocator,
        ctx.device,
        VK_FORMAT_D32_SFLOAT,
        extent,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT);
}

void vk_immediate_submit(
    const VkContext& ctx,
    std::function<void(VkCommandBuffer)> fn)
{
    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = ctx.queue_families.graphics;
    pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

    VkCommandPool tmp_pool;
    VK_CHECK(vkCreateCommandPool(ctx.device, &pool_info, nullptr, &tmp_pool));

    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = tmp_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer cmd;
    VK_CHECK(vkAllocateCommandBuffers(ctx.device, &alloc_info, &cmd));

    VkCommandBufferBeginInfo begin = {};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VK_CHECK(vkBeginCommandBuffer(cmd, &begin));

    fn(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkSubmitInfo submit = {};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    VK_CHECK(vkQueueSubmit(ctx.graphics_queue, 1, &submit, VK_NULL_HANDLE));
    VK_CHECK(vkQueueWaitIdle(ctx.graphics_queue));

    vkDestroyCommandPool(ctx.device, tmp_pool, nullptr);
}