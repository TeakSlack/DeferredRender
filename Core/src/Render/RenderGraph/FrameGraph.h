#ifndef FRAME_GRAPH_H
#define FRAME_GRAPH_H

#include "RenderResource.h"

struct IRenderPassBase;
struct PassBuilder;

class FrameGraph
{
private:
	std::vector<RGPassNode> m_Passes;
	std::vector<IRenderPassBase*> m_PassData;
	std::vector<RGResourceNode> m_Resources;

	friend class PassBuilder;
};

class PassBuilder
{
public:
	PassBuilder(FrameGraph* graph, RGPassNode* pass, const std::string& name) : m_FrameGraph(graph), m_Pass(pass), m_Name(name) {}

	RGMutableTextureHandle CreateTexture(const TextureDesc& desc);
	RGMutableBufferHandle CreateBuffer(const BufferDesc& desc);

	RGTextureHandle ReadTexture(RGTextureHandle handle);
	RGBufferHandle ReadBuffer(RGBufferHandle handle);

private:
	FrameGraph* m_FrameGraph;
	RGPassNode* m_Pass;
	const std::string m_Name;
};

struct IRenderPassBase
{
	virtual ~IRenderPassBase() = default;
};

template<typename PassData>
struct RenderPass : IRenderPassBase
{
	PassData data;
	RGMutableTextureHandle output;
};

#endif // FRAME_GRAPH_H