#include "Render/SceneRenderer.h"
#include "Render/Binning/MeshBinner.h"
#include "Util/Log.h"

#include <algorithm>

void SceneRenderer::SetDevice(IGpuDevice* device)
{
    m_GpuDevice = device;

    BufferDesc lightDesc;
    lightDesc.Usage = BufferUsage::Storage;
    lightDesc.DebugName = "Light Buffer";
    lightDesc.ByteSize = sizeof(SubmittedLightData) * m_MaxLights;
    m_LightBuffer = device->CreateBuffer(lightDesc);
}

void SceneRenderer::ReleaseGpuResources()
{
    if (!m_GpuDevice)
        return;

    m_GpuDevice = nullptr;
}

void SceneRenderer::Shutdown()
{
    ReleaseGpuResources();
}

void SceneRenderer::BeginFrame()
{
    m_SubmittedPackets.clear();
    m_VisiblePackets.clear();
}

void SceneRenderer::Submit(const RenderPacket& packet)
{
    m_SubmittedPackets.push_back(packet);
}

void SceneRenderer::SubmitLight(const SubmittedLight& light)
{
    m_SubmittedLights.push_back(light);
}

void SceneRenderer::CullLights(const::RenderView& view)
{
    m_VisibleLights.clear();

    for (const SubmittedLight& light : m_SubmittedLights)
    {
        if (light.Type == LightType::Directional)
        {
            m_VisibleLights.push_back(light);
            continue;
        }

        if (view.ViewFrustum.IntersectsAABB(light.BoundsMin, light.BoundsMax))
            m_VisibleLights.push_back(light);
    }
}

SceneRenderer::LightGpuData SceneRenderer::FlushLights(ICommandContext* cmd)
{
    uint32_t count = static_cast<uint32_t>(m_VisibleLights.size());
    if (count == 0) return { m_LightBuffer, 0 };

    // Stack-allocate the GPU structs for this frame
    std::vector<SubmittedLightData> gpuLights(count);
    for (uint32_t i = 0; i < count; ++i)
    {
        const SubmittedLight& sl = m_VisibleLights[i];
        gpuLights[i].Position = sl.Position;
        gpuLights[i].Range = sl.Range;
        gpuLights[i].Color = sl.Color;
        gpuLights[i].InvRangeSq = sl.Range > 0.f ? 1.f / (sl.Range * sl.Range) : 0.f;
        gpuLights[i].Direction = sl.Direction;
        gpuLights[i].Type = static_cast<uint32_t>(sl.Type);
        gpuLights[i].InnerConeCos = sl.InnerConeCos;
        gpuLights[i].OuterConeCos = sl.OuterConeCos;
    }

    cmd->WriteBuffer(m_LightBuffer, gpuLights.data(),
        count * sizeof(SubmittedLightData));

    return { m_LightBuffer, count };
}

void SceneRenderer::RenderView(const ::RenderView& view)
{
    m_VisiblePackets.clear();

    for (RenderPacket& packet : m_SubmittedPackets)
    {
        if (view.ViewFrustum.IntersectsAABB(packet.WorldBoundsMin, packet.WorldBoundsMax))
            m_VisiblePackets.push_back(&packet);
    }

    // Sort by SortKey: opaque front-to-back (minimises overdraw / early-Z wins),
    // transparent back-to-front (correct alpha blending).
    // Both orderings are already encoded in SortKey — ascending sort is correct
    // for both because transparent packets invert their depth bits.
    std::sort(m_VisiblePackets.begin(), m_VisiblePackets.end(),
        [](const RenderPacket* a, const RenderPacket* b)
        {
            return a->SortKey < b->SortKey;
        });
}

void SceneRenderer::EndFrame()
{
    // Reserved for future per-frame GPU sync / upload buffer retire.
}

uint32_t SceneRenderer::RegisterTexture(GpuTexture texture)
{
    CORE_ASSERT(m_Binner, "SetBinner() must be called before RegisterTexture()");
    return m_Binner->RegisterTexture(texture);
}
