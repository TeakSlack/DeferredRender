#ifndef COMMON_HLSLI
#define COMMON_HLSLI

// [[vk::binding(b, s)]] is a DXC extension for explicit SPIR-V descriptor
// set/binding assignment. FXC does not understand the [[ ]] attribute syntax
// at all, so we strip it out when not targeting SPIR-V.
#ifdef __spirv__
#define VK_BINDING(b, s) [[vk::binding(b, s)]]
#else
#define VK_BINDING(b, s)
#endif

#endif // COMMON_HLSLI
