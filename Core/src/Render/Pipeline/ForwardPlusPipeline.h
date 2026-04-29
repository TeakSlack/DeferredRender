#ifndef FORWARD_PLUS_PIPELINE_H
#define FORWARD_PLUS_PIPELINE_H

#include "Render/Pipeline/IRenderPipeline.h"

class ForwardPlusPipeline : public IRenderPipeline
{
public:
	void Init(IGpuDevice* device, uint32_t width, uint32_t height, RenderBackend backend) override;
	void Shutdown(IGpuDevice* device) override;
	void OnResize(uint32_t width, uint32_t height) override;

	RGMutableTextureHandle Render(
		FrameGraph& fg,
		SceneRenderer& renderer,
		MeshBinner& binner,
		const RenderView& view,
		RGMutableTextureHandle backbuffer
	) override;
};

#endif // FORWARD_PLUS_PIPELINE_H