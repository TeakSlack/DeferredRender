#include "vk_debug.h"
#include "logger.h"
#include <cstring>
#include <vector>

// -------------------------------------------------------------------------
// Debug callback — receives messages from the validation layer.
// Severity maps directly onto spdlog levels so the terminal colors match
// the rest of the engine output.
// VERBOSE messages are only emitted when verbose mode is on, keeping the
// default output clean.
// -------------------------------------------------------------------------
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      severity,
    VkDebugUtilsMessageTypeFlagsEXT             type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void*                                       /*user_data*/)
{
    // Prefix the message type so PERFORMANCE warnings stand out
    const char* type_tag =
        (type & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) ? "[perf] " :
        (type & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)  ? "[validation] " :
                                                                    "";

    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        LOG_ERROR_TO("vulkan", "{}{}", type_tag, data->pMessage);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        LOG_WARN_TO("vulkan", "{}{}", type_tag, data->pMessage);
    } else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        // Info is fairly chatty — only show in verbose mode
        LOG_DEBUG_TO("vulkan", "{}{}", type_tag, data->pMessage);
    } else {
        // VERBOSE level — pure noise unless you're debugging the driver
        LOG_VERBOSE("{}{}", type_tag, data->pMessage);
    }

    return VK_FALSE;
}

// -------------------------------------------------------------------------
void vk_populate_debug_messenger_create_info(
    VkDebugUtilsMessengerCreateInfoEXT& info)
{
    info = {};
    info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT    |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType     =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT     |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT  |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = debug_callback;
}

// -------------------------------------------------------------------------
bool vk_check_validation_layer_support()
{
    u32 layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
    std::vector<VkLayerProperties> available(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available.data());

    for (const char* name : VALIDATION_LAYERS) {
        bool found = false;
        for (const auto& props : available) {
            if (strcmp(name, props.layerName) == 0) { found = true; break; }
        }
        if (!found) {
            LOG_ERROR_TO("vulkan", "Validation layer not available: {}", name);
            return false;
        }
    }
    return true;
}

// -------------------------------------------------------------------------
// vkCreateDebugUtilsMessengerEXT is an extension function — it must be
// looked up at runtime via vkGetInstanceProcAddr.
// -------------------------------------------------------------------------
VkResult vk_create_debug_messenger(
    VkInstance                                instance,
    const VkDebugUtilsMessengerCreateInfoEXT* create_info,
    const VkAllocationCallbacks*              allocator,
    VkDebugUtilsMessengerEXT*                 out_messenger)
{
    auto fn = (PFN_vkCreateDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (!fn) return VK_ERROR_EXTENSION_NOT_PRESENT;
    return fn(instance, create_info, allocator, out_messenger);
}

void vk_destroy_debug_messenger(
    VkInstance                   instance,
    VkDebugUtilsMessengerEXT     messenger,
    const VkAllocationCallbacks* allocator)
{
    auto fn = (PFN_vkDestroyDebugUtilsMessengerEXT)
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (fn) fn(instance, messenger, allocator);
}
