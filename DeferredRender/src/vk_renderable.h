#ifndef VK_RENDERABLE_H
#define VK_RENDERABLE_H

#include "vk_types.h"
#include "vk_mesh.h"
#include "vk_material.h"

#include <glm/glm.hpp>

// -------------------------------------------------------------------------
// DrawContext - everything a draw call needs that is constant for a pass.
// Passed into IRenderable::draw by the renderer; never stored.
// -------------------------------------------------------------------------
struct DrawContext {
    VkCommandBuffer  cmd = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    u32              frame_index = 0;

    // Set 0 is already bound by the renderer before the draw loop starts
    // (camera UBO, lights, etc). Set 1 is bound per-material inside draw().
    // The pipeline is also already bound before the draw loop.
};

// -------------------------------------------------------------------------
// IRenderable - the only thing the renderer calls per object.
// Implementations bind their material descriptor set, push their transform,
// bind their vertex/index buffers, and issue vkCmdDrawIndexed.
// -------------------------------------------------------------------------
class IRenderable {
public:
    virtual ~IRenderable() = default;

    // Record draw commands into ctx.cmd.
    // Preconditions (guaranteed by caller):
    //   - Correct VkPipeline is bound for this pass
    //   - Set 0 (per-frame) is bound
    //   - Render pass is active
    virtual void draw(const DrawContext& ctx) const = 0;
};

// -------------------------------------------------------------------------
// RenderObject - the standard scene-side renderable.
// Holds a mesh pointer, material pointer, and a world transform.
// Does NOT own mesh or material - both are owned by their respective caches.
// -------------------------------------------------------------------------
struct RenderObject : public IRenderable {
    const Mesh* mesh = nullptr;
    const Material* material = nullptr;
    glm::mat4       transform = glm::mat4(1.0f);

    void draw(const DrawContext& ctx) const override;
};

#endif // VK_RENDERABLE_H