#include "Render/SceneRenderer.h"
#include "Asset/AssetManager.h"
#include "Render/Vertex.h"
#include "Util/Log.h"

#include <algorithm>

void SceneRenderer::SetDevice(IGpuDevice* device)
{
    m_GpuDevice    = device;
    m_UploadContext = device->CreateCommandContext();
}

void SceneRenderer::ReleaseGpuResources()
{
    if (!m_GpuDevice)
        return;

    m_UploadContext.reset();

    for (auto& [id, mesh] : m_MeshCache)
    {
        m_GpuDevice->DestroyBuffer(mesh.VertexBuffer);
        m_GpuDevice->DestroyBuffer(mesh.IndexBuffer);
    }
    m_MeshCache.clear();

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
    if (!EnsureMeshUploaded(packet.Mesh))
        return; // asset still pending — skip this frame

    m_SubmittedPackets.push_back(packet);
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

const SceneRenderer::GpuMesh* SceneRenderer::GetGpuMesh(AssetHandle<MeshAsset> handle) const
{
    auto it = m_MeshCache.find(handle.id);
    if (it == m_MeshCache.end())
        return nullptr;
    return &it->second;
}

bool SceneRenderer::EnsureMeshUploaded(AssetHandle<MeshAsset> handle)
{
    if (m_MeshCache.count(handle.id))
        return true; // already uploaded

	AssetManager* assetManager = Engine::Get().GetSubmodule<AssetManager>();
    MeshAsset* asset = assetManager->GetAsset(handle);
    if (!asset)
        return false; // still pending or failed

    if (asset->Vertices.empty() || asset->Indices.empty())
    {
        CORE_WARN("[SceneRenderer] Skipping empty mesh '{}'", asset->Name);
        return false;
    }

    GpuMesh gpuMesh;
    gpuMesh.IndexCount = static_cast<uint32_t>(asset->Indices.size());

    const uint32_t vbBytes = static_cast<uint32_t>(asset->Vertices.size() * sizeof(Vertex));
    const uint32_t ibBytes = static_cast<uint32_t>(asset->Indices.size()  * sizeof(uint32_t));

    std::string vbName = "VB_" + asset->Name;
    std::string ibName = "IB_" + asset->Name;

    BufferDesc vbDesc;
    vbDesc.byteSize  = vbBytes;
    vbDesc.usage     = BufferUsage::Vertex;
    vbDesc.debugName = vbName.c_str();
    gpuMesh.VertexBuffer = m_GpuDevice->CreateBuffer(vbDesc);

    BufferDesc ibDesc;
    ibDesc.byteSize  = ibBytes;
    ibDesc.usage     = BufferUsage::Index;
    ibDesc.debugName = ibName.c_str();
    gpuMesh.IndexBuffer = m_GpuDevice->CreateBuffer(ibDesc);

    m_UploadContext->Open();
    m_UploadContext->WriteBuffer(gpuMesh.VertexBuffer, asset->Vertices.data(), vbBytes);
    m_UploadContext->WriteBuffer(gpuMesh.IndexBuffer,  asset->Indices.data(),  ibBytes);
    m_UploadContext->Close();
    m_GpuDevice->ExecuteCommandContext(*m_UploadContext);
    m_GpuDevice->WaitForIdle();


    m_MeshCache.emplace(handle.id, std::move(gpuMesh));
    return true;
}
