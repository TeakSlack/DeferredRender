#ifndef SCENE_RENDERER_H
#define SCENE_RENDERER_H

#include <vector>
#include <unordered_map>

#include "Engine.h"
#include "Render/RenderPacket.h"
#include "Render/RenderView.h"
#include "Render/IGpuDevice.h"
#include "Asset/Asset.h"
#include "Util/UUID.h"

// -------------------------------------------------------------------------
// SceneRenderer — IEngineSubmodule responsible for the full render pipeline.
//
// Frame contract (called by AppLayer each frame):
//
//   renderer.BeginFrame();
//
//   SceneRenderSystem::CollectAndSubmit(*scene, cameraPos, renderer);
//                                          ↑ iterates ECS, calls Submit()
//
//   renderer.RenderView(mainView);         // culls, sorts, draws
//   renderer.RenderView(shadowView);       // optional extra views
//
//   renderer.EndFrame();
//
// The renderer never touches entt or Scene directly — SceneRenderSystem is
// the only bridge between the ECS and the renderer.
// -------------------------------------------------------------------------
class SceneRenderer : public IEngineSubmodule
{
public:
    // Uploaded GPU buffers for a single mesh.
    // Declared first so it can be used as a return type below.
    struct GpuMesh
    {
        GpuBuffer VertexBuffer;
        GpuBuffer IndexBuffer;
        uint32_t  IndexCount = 0;
    };

    SceneRenderer() : IEngineSubmodule("SceneRenderer") {}

    // IEngineSubmodule
    void Init()      override {}
    void Shutdown()  override;
    void Tick(float) override {}

    // Call once after the GPU device is ready (typically from AppLayer::OnAttach).
    void SetDevice(IGpuDevice* device);

    // Destroy all GPU-side cached resources.  Call before destroying the device
    // (e.g. from AppLayer::DestroyGpuResources or SwitchBackend).
    void ReleaseGpuResources();

    // ---- Per-frame API --------------------------------------------------

    // Clears the submitted packet list.  Call at the start of each frame.
    void BeginFrame();

    // Queue one packet for this frame.  Called by SceneRenderSystem.
    // If the mesh asset is still Pending, the packet is silently dropped —
    // it will appear automatically next frame once the async load completes.
    void Submit(const RenderPacket& packet);

    // Cull submitted packets against view.ViewFrustum and sort by SortKey.
    // Fills the visible packet list — call GetVisiblePackets() afterwards to
    // iterate draw calls.  Can be called multiple times per frame.
    void RenderView(const ::RenderView& view);

    // Returns the sorted, culled packet list produced by the last RenderView call.
    const std::vector<RenderPacket*>& GetVisiblePackets() const { return m_VisiblePackets; }

    // Resolves a mesh handle to its uploaded GPU buffers.
    // Returns nullptr if the mesh hasn't been uploaded yet (still Pending).
    const GpuMesh* GetGpuMesh(AssetHandle<MeshAsset> handle) const;

    // Call at the end of each frame (currently a no-op; reserved for
    // GPU sync and per-frame cleanup as the pipeline grows).
    void EndFrame();

private:
    // Upload a mesh to GPU on first use.  Returns false if the asset is
    // still loading (Pending) or failed.
    bool EnsureMeshUploaded(AssetHandle<MeshAsset> handle);

    // ---- GPU mesh cache -------------------------------------------------
    // Keyed by AssetID so it survives scene transitions and asset reloads.
    // The renderer owns these GPU buffers for the lifetime of the device.
    std::unordered_map<AssetID, GpuMesh, std::hash<CoreUUID>> m_MeshCache;

    // ---- Per-frame packet storage ---------------------------------------
    // Cleared each BeginFrame.  m_VisiblePackets is a sorted subset filled
    // during RenderView — declared here to avoid per-frame heap allocation.
    std::vector<RenderPacket>  m_SubmittedPackets;
    std::vector<RenderPacket*> m_VisiblePackets;

    // Flush all pending mesh uploads to the GPU. Called at the start of
    // RenderView so all uploads that accumulated during Submit() are sent
    // in a single execute+wait rather than one per mesh.
    void FlushUploads();

    IGpuDevice*                          m_GpuDevice     = nullptr;
    std::unique_ptr<ICommandContext>     m_UploadContext;
    bool                                 m_UploadPending = false;
};

#endif // SCENE_RENDERER_H
