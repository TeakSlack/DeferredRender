#ifndef VK_TEXTURE_H
#define VK_TEXTURE_H

#include "vk_types.h"
#include "vk_buffer.h"

struct Texture
{
	AllocatedImage image;
	VkSampler sampler;
};


Texture load_texture(const VkContext& ctx, const char* path, VkSampler shared_sampler);
void destroy_texture(const VkContext& ctx, Texture& tex);

Texture create_missing_texture(const VkContext& ctx, VkSampler shared_sampler);

VkSampler vk_create_default_sampler(VkDevice device, float max_anisotropy);

#endif // VK_TEXTURE_H