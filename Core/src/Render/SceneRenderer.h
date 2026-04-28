#ifndef SCENE_RENDERER_H
#define SCENE_RENDERER_H

#include <vector>

#include "Engine.h"
#include "Render/RenderPacket.h"
#include "Render/RenderView.h"
#include "Render/IGpuDevice.h"
#include "Render/SubmittedLight.h"

class MeshBinner;

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
    SceneRenderer() : IEngineSubmodule("SceneRenderer") {}

    // IEngineSubmodule
    void Init()      override {}
    void Shutdown()  override;
    void Tick(float) override {}

    // Call once after the GPU device is ready (typically from AppLayer::OnAttach).
    void SetDevice(IGpuDevice* device);
    // Provide the binner whose texture table shadow maps should be registered into.
    void SetBinner(MeshBinner* binner) { m_Binner = binner; }

    // Destroy all GPU-side cached resources.  Call before destroying the device
    // (e.g. from AppLayer::DestroyGpuResources or SwitchBackend).
    void ReleaseGpuResources();

    // ---- Per-frame API --------------------------------------------------

    // Clears the submitted packet list.  Call at the start of each frame.
    void BeginFrame();

    // Queue one packet for this frame.  Called by SceneRenderSystem.
    // Pending mesh assets are handled by MeshBinner::Build — they are silently
    // skipped and will appear automatically once the async load completes.
    void Submit(const RenderPacket& packet);
	void SubmitLight(const SubmittedLight& light);

    // Culls submitted lights against view frusum
    // Call after RenderView - reuses the same view frustum
	// Directional lights always survive culling since they affect the whole scene
	void CullLights(const ::RenderView& view);

	// Returns the list of visible lights after culling. Call after CullLights.
	const std::vector<SubmittedLight>& GetVisibleLights() const { return m_VisibleLights; }

    // GPU buffer management — call once per frame after CullLights().
    // Uploads SubmittedLightData[] to the persistent light buffer.
    // Returns the GpuBuffer and count for FrameGraph import.
    struct LightGpuData { GpuBuffer Buffer; uint32_t Count; };
    LightGpuData FlushLights(ICommandContext* cmd);

    // Cull submitted packets against view.ViewFrustum and sort by SortKey.
    // Fills the visible packet list — call GetVisiblePackets() afterwards to
    // iterate draw calls.  Can be called multiple times per frame.
    void RenderView(const ::RenderView& view);

    // Returns the sorted, culled packet list produced by the last RenderView call.
    const std::vector<RenderPacket*>& GetVisiblePackets() const { return m_VisiblePackets; }

    // Call at the end of each frame (currently a no-op; reserved for
    // GPU sync and per-frame cleanup as the pipeline grows).
    void EndFrame();
    // Register a shadow map (raw GPU texture) into the binner's bindless table.
    uint32_t RegisterTexture(GpuTexture texture);

private:
    // ---- Per-frame packet storage ---------------------------------------
    // Cleared each BeginFrame.  m_VisiblePackets is a sorted subset filled
    // during RenderView — declared here to avoid per-frame heap allocation.
    std::vector<RenderPacket>  m_SubmittedPackets;
    std::vector<RenderPacket*> m_VisiblePackets;

    // ---- Per-frame light storage ----------------------------------------
    std::vector<SubmittedLight> m_SubmittedLights;
    std::vector<SubmittedLight> m_VisibleLights;
	GpuBuffer m_LightBuffer; // persistent GPU buffer for light data, updated each frame
    uint32_t m_MaxLights = 1024;

    IGpuDevice*         m_GpuDevice = nullptr;
    MeshBinner*         m_Binner    = nullptr;

};

#endif // SCENE_RENDERER_H