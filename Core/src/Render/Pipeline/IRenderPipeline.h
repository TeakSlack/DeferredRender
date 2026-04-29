#ifndef RENDER_PIPELINE_H
#define RENDER_PIPELINE_H

#include "Render/IGpuDevice.h"
#include "Render/FrameGraph/FrameGraph.h"
#include "Render/SceneRenderer.h"
#include "Render/Binning/MeshBinner.h"
#include "Render/NvrhiGpuDevice.h"

class IRenderPipeline
{
	virtual ~IRenderPipeline() = default;

	virtual void Init(IGpuDevice* device, uint32_t width, uint32_t height, RenderBackend backend) = 0;
	virtual void Shutdown(IGpuDevice* device) = 0;
	virtual void OnResize(uint32_t width, uint32_t height) = 0;

	virtual RGMutableTextureHandle Render(
		FrameGraph& fg,
		SceneRenderer& renderer,
		MeshBinner& binner,
		const RenderView& view,
		RGMutableTextureHandle backbuffer
	) = 0;
};

#endif // RENDER_PIPELINE_H