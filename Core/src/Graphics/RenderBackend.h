#ifndef RENDER_BACKEND_H
#define RENDER_BACKEND_H

// Backend identifier used for runtime selection and hot-swap
enum class RenderBackend
{
    None,
    Vulkan,
    D3D12,
};

#endif // RENDER_BACKEND_H
