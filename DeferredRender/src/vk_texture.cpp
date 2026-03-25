#include "vk_texture.h"
#include "vk_buffer.h"
#include "stb_image.h"

Texture load_texture(const VkContext& ctx, const char* path, VkSampler shared_sampler)
{
    // --- decode with stb_image ---
    int w = 0, h = 0, channels = 0;
    // Force RGBA even if the source image has fewer channels - uniform format
    // simplifies the descriptor set layout: every albedo slot is R8G8B8A8_SRGB.
    stbi_uc* pixels = stbi_load(path, &w, &h, &channels, STBI_rgb_alpha);
    if (!pixels) {
        LOG_ERROR_TO("render", "Failed to load texture '{}': {}", path, stbi_failure_reason());
        // Return a magenta 1x1 error texture so the renderer stays alive
        return create_missing_texture(ctx, shared_sampler);
    }

    VkDeviceSize image_size = (VkDeviceSize)w * h * 4; // RGBA = 4 bytes

    LOG_VERBOSE("Texture loaded from disk: {} ({}x{} channels={})",
        path, w, h, channels);

    // --- staging buffer ---
    AllocatedBuffer staging = vk_create_staging_buffer(ctx.allocator, pixels, image_size);
    stbi_image_free(pixels);

    // --- device-local image ---
    VkExtent2D extent = { (u32)w, (u32)h };
    Texture tex;
    tex.image = vk_create_image(
        ctx.allocator, ctx.device,
        VK_FORMAT_R8G8B8A8_SRGB,
        extent,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);
    tex.sampler = shared_sampler;

    // --- upload via one-shot command buffer ---
    vk_immediate_submit(ctx, [&](VkCommandBuffer cmd) {
        // UNDEFINED -> TRANSFER_DST_OPTIMAL
        vk_transition_image_layout(cmd, tex.image.image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0; // tightly packed
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { (u32)w, (u32)h, 1 };

        vkCmdCopyBufferToImage(cmd, staging.buffer, tex.image.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        // TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
        vk_transition_image_layout(cmd, tex.image.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        });

    vk_destroy_buffer(ctx.allocator, staging);

    LOG_INFO_TO("render", "Texture uploaded to GPU: {} ({}x{})", path, w, h);
    return tex;
}

Texture create_missing_texture(
    const VkContext& ctx,
    VkSampler        shared_sampler)
{
    // 1×1 pure white pixel - multiplying by this in shaders is a no-op,
    // so materials with no texture assigned work correctly.
    const u8 white[4] = { 255, 255, 255, 255 };
    AllocatedBuffer staging = vk_create_staging_buffer(ctx.allocator, white, 4);

    Texture tex;
    tex.image = vk_create_image(
        ctx.allocator, ctx.device,
        VK_FORMAT_R8G8B8A8_SRGB,
        { 1, 1 },
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);
    tex.sampler = shared_sampler;

    vk_immediate_submit(ctx, [&](VkCommandBuffer cmd) {
        vk_transition_image_layout(cmd, tex.image.image,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkBufferImageCopy region = {};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = { 1, 1, 1 };

        vkCmdCopyBufferToImage(cmd, staging.buffer, tex.image.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        vk_transition_image_layout(cmd, tex.image.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        });

    vk_destroy_buffer(ctx.allocator, staging);
    return tex;
}

void destroy_texture(const VkContext& ctx, Texture& tex)
{
    // Don't destroy the sampler - it's shared and owned by the TextureCache
    vk_destroy_image(ctx.allocator, ctx.device, tex.image);
}

VkSampler vk_create_default_sampler(VkDevice device, float max_anisotropy)
{
    VkSamplerCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = VK_FILTER_LINEAR;
    info.minFilter = VK_FILTER_LINEAR;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.mipLodBias = 0.0f;
    info.anisotropyEnable = max_anisotropy > 1.0f ? VK_TRUE : VK_FALSE;
    info.maxAnisotropy = max_anisotropy;
    info.compareEnable = VK_FALSE;
    info.compareOp = VK_COMPARE_OP_ALWAYS;
    info.minLod = 0.0f;
    info.maxLod = VK_LOD_CLAMP_NONE;  // all mip levels
    info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    info.unnormalizedCoordinates = VK_FALSE;

    VkSampler sampler;
    VK_CHECK(vkCreateSampler(device, &info, nullptr, &sampler));
    return sampler;
}