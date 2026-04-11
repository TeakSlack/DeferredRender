#include "FrameGraph.h"


RGMutableTextureHandle PassBuilder::CreateTexture(const TextureDesc& desc)
{
    // Allocate new resource node within FrameGraph's flat table
	uint16_t index = static_cast<uint16_t>(m_FrameGraph->m_Resources.size());
	RGResourceNode node = m_FrameGraph->m_Resources.emplace_back();
	node.TextureDesc = desc;
	node.Name = desc.debugName;
	node.FirstPassIndex = static_cast<uint16_t>(m_FrameGraph->m_Passes.size()) - 1;

	// Tell the current pass it creates the resource
	m_Pass->Creates.push_back(index);

	// Return a typed, versioned handle to resource
	RGMutableTextureHandle handle;
	handle.Index = index;
	handle.Version = 0;
	return handle;
}

RGMutableBufferHandle PassBuilder::CreateBuffer(const BufferDesc& desc)
{
	// Allocate new resource node within FrameGraph's flat table
	uint16_t index = static_cast<uint16_t>(m_FrameGraph->m_Resources.size());
	RGResourceNode node = m_FrameGraph->m_Resources.emplace_back();
	node.TextureDesc = desc;
	node.Name = desc.debugName;
	node.FirstPassIndex = static_cast<uint16_t>(m_FrameGraph->m_Passes.size()) - 1;

	// Tell the current pass it creates the resource
	m_Pass->Creates.push_back(index);

	// Return a typed, versioned handle to resource
	RGMutableBufferHandle handle;
	handle.Index = index;
	handle.Version = 0;
	return handle;
}
