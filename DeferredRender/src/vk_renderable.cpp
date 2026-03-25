#include "vk_renderable.h"
#include <glm/glm.hpp>

void RenderObject::draw(const DrawContext& ctx) const
{
    // -----------------------------------------------------------------------
    // Guard: skip if either resource is missing. In a complete engine this
    // would log a warning and use the fallback material/mesh, but for Phase 3
    // it's enough to bail early so the frame doesn't crash.
    // -----------------------------------------------------------------------
    if (!mesh || !material) {
        LOG_WARN_TO("render", "RenderObject::draw called with null mesh or material");
        return;
    }

    // -----------------------------------------------------------------------
    // 1. Bind per-material descriptor set (set = 1).
    //    Set 0 (camera/frame data) was already bound before the draw loop.
    //    We do NOT rebind set 0 here - that would reset the binding and is slow.
    // -----------------------------------------------------------------------
    vkCmdBindDescriptorSets(
        ctx.cmd,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        ctx.pipeline_layout,
        1,                                          // firstSet = 1
        1,                                          // setCount
        &material->descriptor_sets[ctx.frame_index],
        0, nullptr);                                // no dynamic offsets

    // -----------------------------------------------------------------------
    // 2. Push per-draw constants: world transform + material tint.
    //    Push constants are the cheapest way to get per-object data to the
    //    shader without any descriptor overhead.
    //
    //    Layout (must match the pipeline layout declaration and the shader):
    //      offset 0,  size 64: mat4  model  (vertex stage)
    //      offset 64, size 16: vec4  albedo_tint (fragment stage)
    // -----------------------------------------------------------------------
    vkCmdPushConstants(
        ctx.cmd,
        ctx.pipeline_layout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,              // offset
        sizeof(glm::mat4),
        &transform);

    vkCmdPushConstants(
        ctx.cmd,
        ctx.pipeline_layout,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        sizeof(glm::mat4),   // offset after the mat4
        sizeof(MaterialPushConstants),
        &material->push_constants);

    // -----------------------------------------------------------------------
    // 3. Bind vertex and index buffers.
    //    Both are DEVICE_LOCAL and were uploaded during mesh creation.
    // -----------------------------------------------------------------------
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(ctx.cmd, 0, 1, &mesh->vertex_buffer.buffer, &offset);
    vkCmdBindIndexBuffer(ctx.cmd, mesh->index_buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

    // -----------------------------------------------------------------------
    // 4. Draw.
    //    instanceCount = 1; Phase 8 can change this for GPU instancing.
    // -----------------------------------------------------------------------
    vkCmdDrawIndexed(ctx.cmd, mesh->index_count, 1, 0, 0, 0);
}