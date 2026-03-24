#ifndef VK_RENDER_PASS_H
#define VK_RENDER_PASS_H

#include "vk_types.h"

struct VkRenderPass {
	VkRenderPass handle = VK_NULL_HANDLE;
	VkAttachmentDescription colorAttachment = {};
};

#endif // VK_RENDER_PASS_H