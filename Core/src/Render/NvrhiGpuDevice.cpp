#include "NvrhiGpuDevice.h"
#include "GpuTypes.h"
#include "Util/Log.h"

// ============================================================================
// Type-conversion helpers  (our types → NVRHI types)
// ============================================================================

static nvrhi::Format ToNvrhiFormat(GpuFormat f)
{
    switch (f)
    {
    case GpuFormat::R8_UNORM:     return nvrhi::Format::R8_UNORM;
    case GpuFormat::RGBA8_UNORM:  return nvrhi::Format::RGBA8_UNORM;
    case GpuFormat::RGBA8_SNORM:  return nvrhi::Format::RGBA8_SNORM;
    case GpuFormat::RGBA8_SRGB:   return nvrhi::Format::SRGBA8_UNORM;
    case GpuFormat::BGRA8_UNORM:  return nvrhi::Format::BGRA8_UNORM;
    case GpuFormat::BGRA8_SRGB:   return nvrhi::Format::SBGRA8_UNORM;
    case GpuFormat::R16_UINT:     return nvrhi::Format::R16_UINT;
    case GpuFormat::R16_FLOAT:    return nvrhi::Format::R16_FLOAT;
    case GpuFormat::RG16_FLOAT:   return nvrhi::Format::RG16_FLOAT;
    case GpuFormat::RGBA16_FLOAT: return nvrhi::Format::RGBA16_FLOAT;
    case GpuFormat::R32_UINT:     return nvrhi::Format::R32_UINT;
    case GpuFormat::R32_FLOAT:    return nvrhi::Format::R32_FLOAT;
    case GpuFormat::RG32_FLOAT:   return nvrhi::Format::RG32_FLOAT;
    case GpuFormat::RGB32_FLOAT:  return nvrhi::Format::RGB32_FLOAT;
    case GpuFormat::RGBA32_FLOAT: return nvrhi::Format::RGBA32_FLOAT;
    case GpuFormat::D16:          return nvrhi::Format::D16;
    case GpuFormat::D24S8:        return nvrhi::Format::D24S8;
    case GpuFormat::D32:          return nvrhi::Format::D32;
    case GpuFormat::D32S8:        return nvrhi::Format::D32S8;
    case GpuFormat::BC1_UNORM:    return nvrhi::Format::BC1_UNORM;
    case GpuFormat::BC1_SRGB:     return nvrhi::Format::BC1_UNORM_SRGB;
    case GpuFormat::BC3_UNORM:    return nvrhi::Format::BC3_UNORM;
    case GpuFormat::BC3_SRGB:     return nvrhi::Format::BC3_UNORM_SRGB;
    case GpuFormat::BC4_UNORM:    return nvrhi::Format::BC4_UNORM;
    case GpuFormat::BC5_UNORM:    return nvrhi::Format::BC5_UNORM;
    case GpuFormat::BC7_UNORM:    return nvrhi::Format::BC7_UNORM;
    case GpuFormat::BC7_SRGB:     return nvrhi::Format::BC7_UNORM_SRGB;
    default:                      return nvrhi::Format::UNKNOWN;
    }
}

// ShaderStage bit values match nvrhi::ShaderType exactly — safe to cast.
static nvrhi::ShaderType ToNvrhiShaderType(ShaderStage s)
{
    return static_cast<nvrhi::ShaderType>(static_cast<uint32_t>(s));
}

static nvrhi::PrimitiveType ToNvrhiPrimType(PrimitiveType p)
{
    switch (p)
    {
    case PrimitiveType::TriangleStrip: return nvrhi::PrimitiveType::TriangleStrip;
    case PrimitiveType::PointList:     return nvrhi::PrimitiveType::PointList;
    case PrimitiveType::LineList:      return nvrhi::PrimitiveType::LineList;
    case PrimitiveType::LineStrip:     return nvrhi::PrimitiveType::LineStrip;
    default:                           return nvrhi::PrimitiveType::TriangleList;
    }
}

static nvrhi::RasterCullMode ToNvrhiCullMode(CullMode c)
{
    switch (c)
    {
    case CullMode::None:  return nvrhi::RasterCullMode::None;
    case CullMode::Front: return nvrhi::RasterCullMode::Front;
    default:              return nvrhi::RasterCullMode::Back;
    }
}

static nvrhi::ComparisonFunc ToNvrhiCompFunc(ComparisonFunc f)
{
    switch (f)
    {
    case ComparisonFunc::Never:          return nvrhi::ComparisonFunc::Never;
    case ComparisonFunc::Less:           return nvrhi::ComparisonFunc::Less;
    case ComparisonFunc::Equal:          return nvrhi::ComparisonFunc::Equal;
    case ComparisonFunc::LessOrEqual:    return nvrhi::ComparisonFunc::LessOrEqual;
    case ComparisonFunc::Greater:        return nvrhi::ComparisonFunc::Greater;
    case ComparisonFunc::NotEqual:       return nvrhi::ComparisonFunc::NotEqual;
    case ComparisonFunc::GreaterOrEqual: return nvrhi::ComparisonFunc::GreaterOrEqual;
    default:                             return nvrhi::ComparisonFunc::Always;
    }
}

static nvrhi::StencilOp ToNvrhiStencilOp(StencilOp op)
{
    switch (op)
    {
    case StencilOp::Zero:     return nvrhi::StencilOp::Zero;
    case StencilOp::Replace:  return nvrhi::StencilOp::Replace;
    case StencilOp::IncrSat:  return nvrhi::StencilOp::IncrementAndClamp;
    case StencilOp::DecrSat:  return nvrhi::StencilOp::DecrementAndClamp;
    case StencilOp::Invert:   return nvrhi::StencilOp::Invert;
    case StencilOp::IncrWrap: return nvrhi::StencilOp::IncrementAndWrap;
    case StencilOp::DecrWrap: return nvrhi::StencilOp::DecrementAndWrap;
    default:                  return nvrhi::StencilOp::Keep;
    }
}

static nvrhi::BlendFactor ToNvrhiBlendFactor(BlendFactor f)
{
    switch (f)
    {
    case BlendFactor::One:             return nvrhi::BlendFactor::One;
    case BlendFactor::SrcColor:        return nvrhi::BlendFactor::SrcColor;
    case BlendFactor::InvSrcColor:     return nvrhi::BlendFactor::OneMinusSrcColor;
    case BlendFactor::SrcAlpha:        return nvrhi::BlendFactor::SrcAlpha;
    case BlendFactor::InvSrcAlpha:     return nvrhi::BlendFactor::OneMinusSrcAlpha;
    case BlendFactor::DstColor:        return nvrhi::BlendFactor::DstColor;
    case BlendFactor::InvDstColor:     return nvrhi::BlendFactor::OneMinusDstColor;
    case BlendFactor::DstAlpha:        return nvrhi::BlendFactor::DstAlpha;
    case BlendFactor::InvDstAlpha:     return nvrhi::BlendFactor::OneMinusDstAlpha;
    case BlendFactor::ConstantColor:   return nvrhi::BlendFactor::ConstantColor;
    case BlendFactor::InvConstantColor:return nvrhi::BlendFactor::OneMinusConstantColor;
    default:                           return nvrhi::BlendFactor::Zero;
    }
}

static nvrhi::BlendOp ToNvrhiBlendOp(BlendOp op)
{
    switch (op)
    {
    case BlendOp::Subtract:    return nvrhi::BlendOp::Subtract;
    case BlendOp::RevSubtract: return nvrhi::BlendOp::ReverseSubtract;
    case BlendOp::Min:         return nvrhi::BlendOp::Min;
    case BlendOp::Max:         return nvrhi::BlendOp::Max;
    default:                   return nvrhi::BlendOp::Add;
    }
}

static nvrhi::SamplerAddressMode ToNvrhiAddressMode(AddressMode m)
{
    switch (m)
    {
    case AddressMode::Mirror: return nvrhi::SamplerAddressMode::Mirror;
    case AddressMode::Clamp:  return nvrhi::SamplerAddressMode::Clamp;
    case AddressMode::Border: return nvrhi::SamplerAddressMode::Border;
    default:                  return nvrhi::SamplerAddressMode::Wrap;
    }
}

static nvrhi::TextureDimension ToNvrhiTexDim(TextureDimension d)
{
    switch (d)
    {
    case TextureDimension::Texture1D:     return nvrhi::TextureDimension::Texture1D;
    case TextureDimension::Texture3D:     return nvrhi::TextureDimension::Texture3D;
    case TextureDimension::Texture2DArray:return nvrhi::TextureDimension::Texture2DArray;
    case TextureDimension::TextureCube:   return nvrhi::TextureDimension::TextureCube;
    default:                              return nvrhi::TextureDimension::Texture2D;
    }
}

static nvrhi::ResourceStates ToNvrhiResourceState(ResourceLayout layout)
{
    switch (layout)
    {
    case ResourceLayout::RenderTarget:   return nvrhi::ResourceStates::RenderTarget;
    case ResourceLayout::DepthWrite:     return nvrhi::ResourceStates::DepthWrite;
    case ResourceLayout::DepthRead:      return nvrhi::ResourceStates::DepthRead;
    case ResourceLayout::ShaderResource: return nvrhi::ResourceStates::ShaderResource;
    case ResourceLayout::UnorderedAccess:return nvrhi::ResourceStates::UnorderedAccess;
    case ResourceLayout::CopySource:     return nvrhi::ResourceStates::CopySource;
    case ResourceLayout::CopyDest:       return nvrhi::ResourceStates::CopyDest;
    case ResourceLayout::Present:        return nvrhi::ResourceStates::Present;
    default:                             return nvrhi::ResourceStates::Unknown;
    }
}

// ============================================================================
// NvrhiCommandContext — ICommandContext implementation
// ============================================================================

class NvrhiCommandContext : public ICommandContext
{
public:
    NvrhiCommandContext(NvrhiGpuDevice* device, nvrhi::CommandListHandle cmdList)
        : m_Device(device), m_CmdList(std::move(cmdList)) {}

    void Open()  override { m_CmdList->open();  ResetState(); }
    void Close() override { m_CmdList->close(); }

    // ---- Upload ----
    void WriteBuffer(GpuBuffer dst, const void* data,
                     size_t byteSize, size_t offset) override
    {
        m_CmdList->writeBuffer(m_Device->GetBuffer(dst.id), data, byteSize, offset);
    }

    void WriteTexture(GpuTexture dst, uint32_t arraySlice, uint32_t mipLevel,
                      const void* data, size_t rowPitch, size_t depthPitch) override
    {
        m_CmdList->writeTexture(m_Device->GetTexture(dst.id),
                                 arraySlice, mipLevel, data, rowPitch, depthPitch);
    }

    // ---- Copy ----
    void CopyBuffer(GpuBuffer dst, GpuBuffer src, size_t byteSize,
                    size_t dstOffset, size_t srcOffset) override
    {
        m_CmdList->copyBuffer(m_Device->GetBuffer(dst.id), dstOffset,
                               m_Device->GetBuffer(src.id), srcOffset, byteSize);
    }

    void CopyTexture(GpuTexture dst, GpuTexture src) override
    {
        nvrhi::TextureSlice slice;
        m_CmdList->copyTexture(m_Device->GetTexture(dst.id), slice,
                                m_Device->GetTexture(src.id), slice);
    }

    // ---- Explicit clears ----
    void ClearColor(GpuTexture texture, const ClearValue& c) override
    {
        m_CmdList->clearTextureFloat(m_Device->GetTexture(texture.id),
                                      nvrhi::AllSubresources,
                                      nvrhi::Color(c.R, c.G, c.B, c.A));
    }

    void ClearDepth(GpuTexture texture, float depth) override
    {
        m_CmdList->clearDepthStencilTexture(m_Device->GetTexture(texture.id),
                                             nvrhi::AllSubresources,
                                             true, depth, false, 0);
    }

    void ClearDepthStencil(GpuTexture texture, float depth, uint8_t stencil) override
    {
        m_CmdList->clearDepthStencilTexture(m_Device->GetTexture(texture.id),
                                             nvrhi::AllSubresources,
                                             true, depth, true, stencil);
    }

    // ---- Render pass ----
    // Clears are deferred until the first DrawFlush so setGraphicsState
    // has already bound the framebuffer (required for D3D12/NVRHI).
    void BeginRenderPass(const RenderPassDesc& desc) override
    {
        m_GfxState.framebuffer = m_Device->GetFramebuffer(desc.Framebuffer.id);
        m_PendingFbHandle      = desc.Framebuffer;
        m_PendingRpDesc        = desc;
        m_HasPendingClears     = desc.ClearColor || desc.ClearDepth || desc.ClearStencil;
        m_GfxStateDirty        = true;
    }

    void EndRenderPass() override
    {
        // FlushGraphicsState is gated on having a pipeline, so clear-only passes
        // (no draws, no SetGraphicsPipeline) would silently drop their clears.
        // Apply them directly here — clearTextureFloat handles its own layout
        // transition internally and does not require an active pipeline.
        if (m_HasPendingClears)
        {
            ApplyPendingClears();
            m_HasPendingClears = false;
        }

        m_GfxState.framebuffer = nullptr;
        m_PendingFbHandle      = {};
        m_PendingRpDesc        = {};
        m_GfxStateDirty        = false;
    }

    // ---- Graphics state ----
    void SetGraphicsPipeline(GpuGraphicsPipeline pipeline) override
    {
        m_GfxState.pipeline = m_Device->GetGraphicsPipeline(pipeline.id);
        m_GfxStateDirty = true;
    }

    void SetViewport(const Viewport& vp) override
    {
        nvrhi::Viewport nvVp(vp.X, vp.X + vp.Width,
                             vp.Y, vp.Y + vp.Height,
                             vp.MinDepth, vp.MaxDepth);
        auto& vps = m_GfxState.viewport.viewports;
        if (vps.empty()) vps.push_back(nvVp); else vps[0] = nvVp;

        // Auto scissor — overridden if SetScissor is called explicitly.
        if (!m_ExplicitScissor)
        {
            nvrhi::Rect nvSr((int)vp.X, (int)(vp.X + vp.Width),
                             (int)vp.Y, (int)(vp.Y + vp.Height));
            auto& srs = m_GfxState.viewport.scissorRects;
            if (srs.empty()) srs.push_back(nvSr); else srs[0] = nvSr;
        }
        m_GfxStateDirty = true;
    }

    void SetViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f)
    {
		SetViewport(Viewport{ x, y, width, height, minDepth, maxDepth });
    }

    void SetScissor(const ScissorRect& rect) override
    {
        nvrhi::Rect nvSr(rect.X, rect.X + rect.Width,
                         rect.Y, rect.Y + rect.Height);
        auto& srs = m_GfxState.viewport.scissorRects;
        if (srs.empty()) srs.push_back(nvSr); else srs[0] = nvSr;
        m_ExplicitScissor = true;
        m_GfxStateDirty   = true;
    }

    void SetScissor(int x, int y, int width, int height)
    {
        SetScissor(ScissorRect{ x, y, width, height });
    }

    void SetVertexBuffer(uint32_t slot, GpuBuffer buffer, uint64_t byteOffset) override
    {
        while (m_GfxState.vertexBuffers.size() <= slot)
            m_GfxState.vertexBuffers.push_back({});
        m_GfxState.vertexBuffers[slot] =
            nvrhi::VertexBufferBinding{ m_Device->GetBuffer(buffer.id), slot, byteOffset };
        m_GfxStateDirty = true;
    }

    void SetIndexBuffer(GpuBuffer buffer, GpuFormat indexFormat,
                        uint64_t byteOffset) override
    {
        m_GfxState.indexBuffer =
            nvrhi::IndexBufferBinding{ m_Device->GetBuffer(buffer.id),
                                       ToNvrhiFormat(indexFormat), static_cast<uint32_t>(byteOffset) };
        m_GfxStateDirty = true;
    }

    void SetBindingSet(GpuBindingSet set, uint32_t slot) override
    {
        while (m_GfxState.bindings.size() <= slot)
            m_GfxState.bindings.push_back(nullptr);
        m_GfxState.bindings[slot] = m_Device->GetBindingSet(set.id);
        m_GfxStateDirty = true;
    }

    // IDescriptorTable derives from IBindingSet in NVRHI, so we can place it
    // directly in the bindings array at the chosen root slot.
    void SetBindlessTable(GpuDescriptorTable table, uint32_t rootSlot) override
    {
        while (m_GfxState.bindings.size() <= rootSlot)
            m_GfxState.bindings.push_back(nullptr);
        m_GfxState.bindings[rootSlot] =
            static_cast<nvrhi::IBindingSet*>(m_Device->GetDescriptorTableNative(table.id));
        m_GfxStateDirty = true;
    }

    // Push constants are implemented via a volatile constant buffer bound at slot 0.
    // Convention: pipelines that use push constants must include the push constant
    // binding layout as index 0 in their GraphicsPipelineDesc::bindingLayouts array.
    void SetGraphicsPushConstants(const void* data, uint32_t byteSize) override
    {
        EnsurePushConstantResources();
        m_CmdList->writeBuffer(m_PushConstantBuffer, data, byteSize);
        if (m_GfxState.bindings.empty())
            m_GfxState.bindings.push_back(nullptr);
        m_GfxState.bindings[0] = m_PushConstantBindingSet;
        m_GfxStateDirty = true;
    }

    void SetComputePushConstants(const void* data, uint32_t byteSize) override
    {
        EnsurePushConstantResources();
        m_CmdList->writeBuffer(m_PushConstantBuffer, data, byteSize);
        if (m_CmpState.bindings.empty())
            m_CmpState.bindings.push_back(nullptr);
        m_CmpState.bindings[0] = m_PushConstantBindingSet;
        m_CmpStateDirty = true;
    }

    // ---- Draw ----
    void Draw(const DrawArgs& args) override
    {
        FlushGraphicsState();
        nvrhi::DrawArguments a;
        a.vertexCount         = args.VertexCount;
        a.instanceCount       = args.InstanceCount;
        a.startVertexLocation = args.StartVertex;
        a.startInstanceLocation = args.StartInstance;
        m_CmdList->draw(a);
    }

    void DrawIndexed(const DrawIndexedArgs& args) override
    {
        FlushGraphicsState();
        nvrhi::DrawArguments a;
        a.vertexCount           = args.IndexCount;
        a.instanceCount         = args.InstanceCount;
        a.startIndexLocation    = args.StartIndex;
        a.startVertexLocation   = args.BaseVertex;
        a.startInstanceLocation = args.StartInstance;
        m_CmdList->drawIndexed(a);
    }

    void DrawIndirect(GpuBuffer argsBuffer, uint64_t byteOffset) override
    {
        m_GfxState.indirectParams = m_Device->GetBuffer(argsBuffer.id);
        m_GfxStateDirty = true;
        FlushGraphicsState();
        m_CmdList->drawIndirect(static_cast<uint32_t>(byteOffset));
        m_GfxState.indirectParams = nullptr;
    }

    void DrawIndexedIndirect(GpuBuffer argsBuffer, uint64_t byteOffset) override
    {
        m_GfxState.indirectParams = m_Device->GetBuffer(argsBuffer.id);
        m_GfxStateDirty = true;
        FlushGraphicsState();
        m_CmdList->drawIndexedIndirect(static_cast<uint32_t>(byteOffset));
        m_GfxState.indirectParams = nullptr;
    }

    // ---- Compute ----
    void SetComputePipeline(GpuComputePipeline pipeline) override
    {
        m_CmpState.pipeline = m_Device->GetComputePipeline(pipeline.id);
        m_CmpStateDirty = true;
    }

    void SetComputeBindingSet(GpuBindingSet set, uint32_t slot) override
    {
        while (m_CmpState.bindings.size() <= slot)
            m_CmpState.bindings.push_back(nullptr);
        m_CmpState.bindings[slot] = m_Device->GetBindingSet(set.id);
        m_CmpStateDirty = true;
    }

    void Dispatch(const DispatchArgs& args) override
    {
        FlushComputeState();
        m_CmdList->dispatch(args.GroupX, args.GroupY, args.GroupZ);
    }

    void DispatchIndirect(GpuBuffer argsBuffer, uint64_t byteOffset) override
    {
        FlushComputeState();
        m_CmdList->dispatchIndirect(byteOffset);
    }

    // ---- Resource transitions ----
    void TransitionTexture(GpuTexture texture, ResourceLayout layout) override
    {
        m_CmdList->setTextureState(m_Device->GetTexture(texture.id),
                                    nvrhi::AllSubresources,
                                    ToNvrhiResourceState(layout));
        m_CmdList->commitBarriers();
    }

    void TransitionBuffer(GpuBuffer buffer, ResourceLayout layout) override
    {
        m_CmdList->setBufferState(m_Device->GetBuffer(buffer.id),
                                   ToNvrhiResourceState(layout));
        m_CmdList->commitBarriers();
    }

    // ---- Debug markers ----
    void BeginMarker(const char* name) override { m_CmdList->beginMarker(name); }
    void EndMarker()                   override { m_CmdList->endMarker(); }

    nvrhi::ICommandList* GetNative() const { return m_CmdList.Get(); }

private:
    void ResetState()
    {
        m_GfxState       = {};
        m_CmpState       = {};
        m_GfxStateDirty  = false;
        m_CmpStateDirty  = false;
        m_ExplicitScissor = false;
        m_HasPendingClears = false;
        m_PendingFbHandle  = {};
        m_PendingRpDesc    = {};
    }

    void FlushGraphicsState()
    {
        if (!m_GfxStateDirty) return;
        if (!m_GfxState.pipeline || !m_GfxState.framebuffer) return;
        m_CmdList->setGraphicsState(m_GfxState);
        m_GfxStateDirty = false;

        if (m_HasPendingClears)
        {
            // Apply deferred clears now that the framebuffer is bound (required
            // for D3D12). On Vulkan, clearTextureFloat issues vkCmdClearColorImage
            // which must be outside a render pass, so NVRHI ends the active pass.
            // We call setGraphicsState again afterwards to restart it before draw.
            ApplyPendingClears();
            m_HasPendingClears = false;
            m_CmdList->setGraphicsState(m_GfxState);
        }
    }

    void FlushComputeState()
    {
        if (!m_CmpStateDirty) return;
        if (!m_CmpState.pipeline) return;
        m_CmdList->setComputeState(m_CmpState);
        m_CmpStateDirty = false;
    }

    // Creates a 256-byte volatile CBV + binding layout + binding set on first use.
    // The binding layout has a single CBV at register b0 (binding offset 0).
    void EnsurePushConstantResources()
    {
        if (m_PushConstantBuffer) return;

        nvrhi::BufferDesc bd;
        bd.byteSize         = 256;
        bd.isConstantBuffer = true;
        bd.isVolatile       = true;
        bd.maxVersions      = 256;
        bd.debugName        = "PushConstants";
        m_PushConstantBuffer = m_Device->GetDevice()->createBuffer(bd);

        nvrhi::BindingLayoutDesc bld;
        bld.visibility = nvrhi::ShaderType::All;
        bld.bindingOffsets.setConstantBufferOffset(0);
        bld.bindings = { nvrhi::BindingLayoutItem::ConstantBuffer(0) };
        m_PushConstantLayout = m_Device->GetDevice()->createBindingLayout(bld);

        nvrhi::BindingSetDesc bsd;
        bsd.bindings = { nvrhi::BindingSetItem::ConstantBuffer(0, m_PushConstantBuffer) };
        m_PushConstantBindingSet =
            m_Device->GetDevice()->createBindingSet(bsd, m_PushConstantLayout);
    }

    void ApplyPendingClears()
    {
        const auto& fbDesc = m_Device->GetFramebufferDesc(m_PendingFbHandle.id);

        if (m_PendingRpDesc.ClearColor)
        {
            auto& cv = m_PendingRpDesc.ColorValue;
            for (const auto& attach : fbDesc.ColorAttachments)
            {
                if (!attach.Texture.IsValid()) continue;
                m_CmdList->clearTextureFloat(
                    m_Device->GetTexture(attach.Texture.id),
                    nvrhi::AllSubresources,
                    nvrhi::Color(cv.R, cv.G, cv.B, cv.A));
            }
        }

        if ((m_PendingRpDesc.ClearDepth || m_PendingRpDesc.ClearStencil)
            && fbDesc.DepthAttachment.Texture.IsValid())
        {
            m_CmdList->clearDepthStencilTexture(
                m_Device->GetTexture(fbDesc.DepthAttachment.Texture.id),
                nvrhi::AllSubresources,
                m_PendingRpDesc.ClearDepth,   m_PendingRpDesc.DepthValue,
                m_PendingRpDesc.ClearStencil, m_PendingRpDesc.StencilValue);
        }
    }

    NvrhiGpuDevice*      m_Device        = nullptr;
    nvrhi::CommandListHandle m_CmdList;

    nvrhi::GraphicsState m_GfxState;
    bool                 m_GfxStateDirty  = false;
    bool                 m_ExplicitScissor = false;

    nvrhi::ComputeState  m_CmpState;
    bool                 m_CmpStateDirty  = false;

    bool           m_HasPendingClears = false;
    GpuFramebuffer m_PendingFbHandle;
    RenderPassDesc m_PendingRpDesc;

    // Push constant resources (lazy-initialized on first SetGraphicsPushConstants call)
    nvrhi::BufferHandle        m_PushConstantBuffer;
    nvrhi::BindingLayoutHandle m_PushConstantLayout;
    nvrhi::BindingSetHandle    m_PushConstantBindingSet;
};

// ============================================================================
// NvrhiGpuDevice — resource creation
// ============================================================================

NvrhiGpuDevice::NvrhiGpuDevice(nvrhi::IDevice* device)
    : m_Device(device)
{}

// ---- Buffers ----

GpuBuffer NvrhiGpuDevice::CreateBuffer(const BufferDesc& desc)
{
    nvrhi::BufferDesc bd;
    bd.byteSize       = desc.ByteSize;
    bd.debugName      = desc.DebugName ? desc.DebugName : "";
    bd.isConstantBuffer   = (desc.Usage & BufferUsage::Constant) != 0;
    bd.isVertexBuffer     = (desc.Usage & BufferUsage::Vertex)   != 0;
    bd.isIndexBuffer      = (desc.Usage & BufferUsage::Index)    != 0;
    bd.isDrawIndirectArgs = (desc.Usage & BufferUsage::Indirect)  != 0;
    if (desc.Usage & BufferUsage::Storage)
        bd.canHaveUAVs = true;
    if (desc.Usage & BufferUsage::Staging)
    {
        bd.cpuAccess  = nvrhi::CpuAccessMode::Write;
        bd.isVertexBuffer = false; // staging buffers are not VBs
    }
    bd.keepInitialState = true;

    // Set initial resource state
    if (desc.Usage & BufferUsage::Constant)
        bd.initialState = nvrhi::ResourceStates::ConstantBuffer;
    else if (desc.Usage & BufferUsage::Vertex)
        bd.initialState = nvrhi::ResourceStates::VertexBuffer;
    else if (desc.Usage & BufferUsage::Index)
        bd.initialState = nvrhi::ResourceStates::IndexBuffer;

    return { m_Buffers.Add(m_Device->createBuffer(bd)) };
}

void NvrhiGpuDevice::DestroyBuffer(GpuBuffer handle)
{
    m_Buffers.Release(handle.id);
}

// ---- Textures ----

GpuTexture NvrhiGpuDevice::CreateTexture(const TextureDesc& desc)
{
    nvrhi::TextureDesc td;
    td.width       = desc.Width;
    td.height      = desc.Height;
    td.depth       = desc.Depth;
    td.mipLevels   = desc.MipLevels;
    td.sampleCount = desc.SampleCount;
    td.format      = ToNvrhiFormat(desc.Format);
    td.dimension   = ToNvrhiTexDim(desc.Dimension);
    td.debugName   = desc.DebugName ? desc.DebugName : "";
    td.isRenderTarget   = (desc.Usage & TextureUsage::RenderTarget) != 0;
    td.isUAV            = (desc.Usage & TextureUsage::Storage)      != 0;
    td.keepInitialState = true;

    if (desc.Usage & TextureUsage::DepthStencil)
    {
        td.isRenderTarget  = true;
        td.initialState    = nvrhi::ResourceStates::DepthWrite;
        td.clearValue      = nvrhi::Color(desc.OptimizedClearDepth);
    }
    else if (desc.Usage & TextureUsage::RenderTarget)
    {
        td.initialState    = nvrhi::ResourceStates::RenderTarget;
        td.clearValue      = nvrhi::Color(desc.OptimizedClearColor.R,
                                           desc.OptimizedClearColor.G,
                                           desc.OptimizedClearColor.B,
                                           desc.OptimizedClearColor.A);
    }
    else
    {
        td.initialState = nvrhi::ResourceStates::ShaderResource;
    }

    return { m_Textures.Add(m_Device->createTexture(td)) };
}

void NvrhiGpuDevice::DestroyTexture(GpuTexture handle)
{
    m_Textures.Release(handle.id);
}

// ---- Samplers ----

GpuSampler NvrhiGpuDevice::CreateSampler(const SamplerDesc& desc)
{
    nvrhi::SamplerDesc sd;
    sd.addressU    = ToNvrhiAddressMode(desc.AddressU);
    sd.addressV    = ToNvrhiAddressMode(desc.AddressV);
    sd.addressW    = ToNvrhiAddressMode(desc.AddressW);
    sd.mipBias       = desc.MipLODBias;
    sd.maxAnisotropy = (float)desc.MaxAnisotropy;
    // Note: NVRHI SamplerDesc has no comparisonFunc or LOD clamp fields.
    // Those fields on our SamplerDesc are silently ignored in this backend.

    bool linear = (desc.MinFilter == Filter::Linear || desc.MagFilter == Filter::Linear);
    bool aniso  = (desc.MinFilter == Filter::Anisotropic);
    sd.minFilter = sd.magFilter = sd.mipFilter = linear || aniso;

    return { m_Samplers.Add(m_Device->createSampler(sd)) };
}

void NvrhiGpuDevice::DestroySampler(GpuSampler handle)
{
    m_Samplers.Release(handle.id);
}

// ---- Shaders ----

GpuShader NvrhiGpuDevice::CreateShader(const ShaderDesc& desc)
{
    nvrhi::ShaderDesc sd;
    sd.shaderType = ToNvrhiShaderType(desc.Stage);
    sd.entryName  = desc.EntryPoint ? desc.EntryPoint : "main";
    sd.debugName  = desc.DebugName  ? desc.DebugName  : "";

    return { m_Shaders.Add(
        m_Device->createShader(sd, desc.Bytecode, desc.ByteSize)) };
}

void NvrhiGpuDevice::DestroyShader(GpuShader handle)
{
    m_Shaders.Release(handle.id);
}

// ---- Input layout ----

GpuInputLayout NvrhiGpuDevice::CreateInputLayout(
    const std::vector<VertexAttributeDesc>& attribs,
    GpuShader vertexShader)
{
    std::vector<nvrhi::VertexAttributeDesc> nvAttribs;
    nvAttribs.reserve(attribs.size());
    for (const auto& a : attribs)
    {
        nvrhi::VertexAttributeDesc va;
        va.name          = a.Name ? a.Name : "";
        va.format        = ToNvrhiFormat(a.Format);
        va.bufferIndex   = a.BufferIndex;
        va.offset        = a.Offset;
        va.elementStride = a.Stride;
        nvAttribs.push_back(va);
    }
    auto nvVs = m_Shaders.Get(vertexShader.id);
    return { m_InputLayouts.Add(
        m_Device->createInputLayout(nvAttribs.data(),
                                     (uint32_t)nvAttribs.size(), nvVs)) };
}

void NvrhiGpuDevice::DestroyInputLayout(GpuInputLayout handle)
{
    m_InputLayouts.Release(handle.id);
}

// ---- Binding layout ----

GpuBindingLayout NvrhiGpuDevice::CreateBindingLayout(const BindingLayoutDesc& desc)
{
    nvrhi::BindingLayoutDesc bld;

    // Aggregate visibility from all items.
    uint32_t vis = 0;
    for (const auto& item : desc.Items)
        vis |= (uint32_t)item.Stage;
    bld.visibility = static_cast<nvrhi::ShaderType>(vis);
    // Vulkan binding = offset + slot.  Slots are D3D12 register indices (t0, s0, b0 ...).
    // Offsets pack the per-type namespaces into a flat Vulkan binding space:
    //   CBV  slot N  → binding N        (offset 0, covers b0..bN)
    //   SRV  slot N  → binding 2+N      (offset 2)
    //   Sampler slot N → binding 3+N    (offset 3)
    //   UAV  slot N  → binding 4+N      (offset 4)
    // These must match the [[vk::binding(X,0)]] annotations in the HLSL shaders.
    bld.bindingOffsets.setConstantBufferOffset(0);
    bld.bindingOffsets.setShaderResourceOffset(2);
    bld.bindingOffsets.setSamplerOffset(3);
    bld.bindingOffsets.setUnorderedAccessViewOffset(4);

    for (const auto& item : desc.Items)
    {
        switch (item.Type)
        {
        case BindingType::ConstantBuffer:
            bld.bindings.push_back(nvrhi::BindingLayoutItem::ConstantBuffer(item.Slot));
            break;
        case BindingType::Texture:
            bld.bindings.push_back(nvrhi::BindingLayoutItem::Texture_SRV(item.Slot));
            break;
        case BindingType::Sampler:
            bld.bindings.push_back(nvrhi::BindingLayoutItem::Sampler(item.Slot));
            break;
        case BindingType::StorageBuffer:
            bld.bindings.push_back(nvrhi::BindingLayoutItem::RawBuffer_UAV(item.Slot));
            break;
        case BindingType::StorageTexture:
            bld.bindings.push_back(nvrhi::BindingLayoutItem::Texture_UAV(item.Slot));
            break;
        }
    }

    return { m_BindingLayouts.Add(m_Device->createBindingLayout(bld)) };
}

void NvrhiGpuDevice::DestroyBindingLayout(GpuBindingLayout handle)
{
    m_BindingLayouts.Release(handle.id);
}

// ---- Binding set ----

GpuBindingSet NvrhiGpuDevice::CreateBindingSet(const BindingSetDesc& desc,
                                                 GpuBindingLayout layout)
{
    nvrhi::BindingSetDesc bsd;
    for (const auto& item : desc.Items)
    {
        switch (item.Type)
        {
        case BindingType::ConstantBuffer:
            bsd.bindings.push_back(
                nvrhi::BindingSetItem::ConstantBuffer(item.Slot,
                    m_Buffers.Get(item.Buffer.id)));
            break;
        case BindingType::Texture:
            bsd.bindings.push_back(
                nvrhi::BindingSetItem::Texture_SRV(item.Slot,
                    m_Textures.Get(item.TextureHandle.id)));
            break;
        case BindingType::Sampler:
            bsd.bindings.push_back(
                nvrhi::BindingSetItem::Sampler(item.Slot,
                    m_Samplers.Get(item.SamplerHandle.id)));
            break;
        case BindingType::StorageBuffer:
            bsd.bindings.push_back(
                nvrhi::BindingSetItem::RawBuffer_UAV(item.Slot,
                    m_Buffers.Get(item.Buffer.id)));
            break;
        case BindingType::StorageTexture:
            bsd.bindings.push_back(
                nvrhi::BindingSetItem::Texture_UAV(item.Slot,
                    m_Textures.Get(item.TextureHandle.id)));
            break;
        }
    }

    auto nvLayout = m_BindingLayouts.Get(layout.id);
    return { m_BindingSets.Add(m_Device->createBindingSet(bsd, nvLayout)) };
}

void NvrhiGpuDevice::DestroyBindingSet(GpuBindingSet handle)
{
    m_BindingSets.Release(handle.id);
}

// ---- Bindless layout ----

GpuBindlessLayout NvrhiGpuDevice::CreateBindlessLayout(const BindlessLayoutDesc& desc)
{
    nvrhi::BindlessLayoutDesc bld;
    bld.visibility   = nvrhi::ShaderType::All;
    bld.firstSlot    = 0;
    bld.maxCapacity  = desc.MaxResources;

    // registerSpaces entries map HLSL register spaces to this descriptor table.
    // Convention: textures→space1, buffers→space2, samplers→space3.
    switch (desc.ResourceType)
    {
    case BindlessResourceType::Texture:
        bld.registerSpaces = { nvrhi::BindingLayoutItem::Texture_SRV(1) };
        break;
    case BindlessResourceType::Buffer:
        bld.registerSpaces = { nvrhi::BindingLayoutItem::StructuredBuffer_SRV(2) };
        break;
    case BindlessResourceType::Sampler:
        bld.registerSpaces = { nvrhi::BindingLayoutItem::Sampler(3) };
        break;
    }

    BindlessLayoutEntry entry;
    entry.maxCapacity = desc.MaxResources;
    entry.handle      = m_Device->createBindlessLayout(bld);
    return { m_BindlessLayouts.Add(std::move(entry)) };
}

void NvrhiGpuDevice::DestroyBindlessLayout(GpuBindlessLayout handle)
{
    m_BindlessLayouts.Release(handle.id);
}

// ---- Descriptor table ----

GpuDescriptorTable NvrhiGpuDevice::CreateDescriptorTable(GpuBindlessLayout layout)
{
    auto& layoutEntry = m_BindlessLayouts.Get(layout.id);
    DescriptorTableEntry entry;
    entry.capacity     = layoutEntry.maxCapacity;
    entry.nextFreeSlot = 0;
    entry.handle       = m_Device->createDescriptorTable(layoutEntry.handle);
    return { m_DescriptorTables.Add(std::move(entry)) };
}

void NvrhiGpuDevice::DestroyDescriptorTable(GpuDescriptorTable handle)
{
    m_DescriptorTables.Release(handle.id);
}

// ---- Descriptor writes ----

DescriptorIndex NvrhiGpuDevice::WriteTexture(GpuDescriptorTable table,
                                               GpuTexture texture,
                                               DescriptorIndex slot)
{
    auto& entry = m_DescriptorTables.Get(table.id);
    if (slot == InvalidDescriptorIndex)
        slot = entry.nextFreeSlot++;
    m_Device->writeDescriptorTable(
        entry.handle,
        nvrhi::BindingSetItem::Texture_SRV(slot, m_Textures.Get(texture.id)));
    return slot;
}

DescriptorIndex NvrhiGpuDevice::WriteBuffer(GpuDescriptorTable table,
                                              GpuBuffer buffer,
                                              DescriptorIndex slot)
{
    auto& entry = m_DescriptorTables.Get(table.id);
    if (slot == InvalidDescriptorIndex)
        slot = entry.nextFreeSlot++;
    m_Device->writeDescriptorTable(
        entry.handle,
        nvrhi::BindingSetItem::StructuredBuffer_SRV(slot, m_Buffers.Get(buffer.id)));
    return slot;
}

DescriptorIndex NvrhiGpuDevice::WriteSampler(GpuDescriptorTable table,
                                               GpuSampler sampler,
                                               DescriptorIndex slot)
{
    auto& entry = m_DescriptorTables.Get(table.id);
    if (slot == InvalidDescriptorIndex)
        slot = entry.nextFreeSlot++;
    m_Device->writeDescriptorTable(
        entry.handle,
        nvrhi::BindingSetItem::Sampler(slot, m_Samplers.Get(sampler.id)));
    return slot;
}

// ---- Framebuffer ----

GpuFramebuffer NvrhiGpuDevice::CreateFramebuffer(const FramebufferDesc& desc)
{
    nvrhi::FramebufferDesc nvDesc;
    for (const auto& attach : desc.ColorAttachments)
    {
        nvrhi::FramebufferAttachment a;
        a.texture    = m_Textures.Get(attach.Texture.id);
        a.setMipLevel(attach.MipLevel);
        a.setArraySlice(attach.ArraySlice);
        nvDesc.addColorAttachment(a);
    }
    if (desc.DepthAttachment.Texture.IsValid())
    {
        nvrhi::FramebufferAttachment a;
        a.texture    = m_Textures.Get(desc.DepthAttachment.Texture.id);
        a.setMipLevel(desc.DepthAttachment.MipLevel);
        a.setArraySlice(desc.DepthAttachment.ArraySlice);
        nvDesc.setDepthAttachment(a);
    }

    uint32_t nvFbId = m_Framebuffers.Add(m_Device->createFramebuffer(nvDesc));
    // Slot IDs must stay in sync between both pools.
    uint32_t descId = m_FramebufferDescs.Add(desc);
    (void)descId;

    return { nvFbId };
}

void NvrhiGpuDevice::DestroyFramebuffer(GpuFramebuffer handle)
{
    m_Framebuffers.Release(handle.id);
    m_FramebufferDescs.Release(handle.id);
}

std::pair<uint32_t, uint32_t> NvrhiGpuDevice::GetFramebufferSize(
    GpuFramebuffer handle) const
{
    auto& fbDesc = m_FramebufferDescs.Get(handle.id);
    if (!fbDesc.ColorAttachments.empty())
    {
        auto nvTex = m_Textures.Get(fbDesc.ColorAttachments[0].Texture.id);
        if (nvTex)
        {
            const auto& td = nvTex->getDesc();
            return { td.width, td.height };
        }
    }
    return { 0, 0 };
}

// ---- Pipelines ----

static nvrhi::RenderState ToNvrhiRenderState(const GraphicsPipelineDesc& desc)
{
    nvrhi::RenderState rs;

    // Rasterizer
    rs.rasterState.fillMode = desc.Rasterizer.FillMode == FillMode::Wireframe
        ? nvrhi::RasterFillMode::Wireframe : nvrhi::RasterFillMode::Fill;
    rs.rasterState.cullMode              = ToNvrhiCullMode(desc.Rasterizer.CullMode);
    rs.rasterState.frontCounterClockwise = desc.Rasterizer.FrontCCW;
    rs.rasterState.depthBias             = desc.Rasterizer.DepthBias;
    rs.rasterState.slopeScaledDepthBias  = desc.Rasterizer.SlopeScaledDepthBias;
    rs.rasterState.depthClipEnable       = desc.Rasterizer.DepthClipEnable;

    // Depth-stencil
    auto& ds = rs.depthStencilState;
    ds.depthTestEnable  = desc.DepthStencil.DepthTestEnable;
    ds.depthWriteEnable = desc.DepthStencil.DepthWriteEnable;
    ds.depthFunc        = ToNvrhiCompFunc(desc.DepthStencil.DepthFunc);
    ds.stencilEnable    = desc.DepthStencil.StencilEnable;
    ds.stencilReadMask  = desc.DepthStencil.StencilReadMask;
    ds.stencilWriteMask = desc.DepthStencil.StencilWriteMask;

    auto cvtStencilOp = [](const StencilOpDesc& s) {
        nvrhi::DepthStencilState::StencilOpDesc o;
        o.failOp      = ToNvrhiStencilOp(s.FailOp);
        o.depthFailOp = ToNvrhiStencilOp(s.DepthFailOp);
        o.passOp      = ToNvrhiStencilOp(s.PassOp);
        o.stencilFunc = ToNvrhiCompFunc(s.Func);
        return o;
    };
    ds.frontFaceStencil = cvtStencilOp(desc.DepthStencil.FrontFace);
    ds.backFaceStencil  = cvtStencilOp(desc.DepthStencil.BackFace);
    // Blend
    for (int t = 0; t < 8; ++t)
    {
        const auto& src = desc.Blend.RenderTargets[t];
        auto&       dst = rs.blendState.targets[t];
        dst.blendEnable    = src.BlendEnable;
        dst.srcBlend       = ToNvrhiBlendFactor(src.SrcBlend);
        dst.destBlend      = ToNvrhiBlendFactor(src.DstBlend);
        dst.blendOp        = ToNvrhiBlendOp(src.BlendOperator);
        dst.srcBlendAlpha  = ToNvrhiBlendFactor(src.SrcBlendAlpha);
        dst.destBlendAlpha = ToNvrhiBlendFactor(src.DstBlendAlpha);
        dst.blendOpAlpha   = ToNvrhiBlendOp(src.BlendOpAlpha);
        dst.colorWriteMask = static_cast<nvrhi::ColorMask>(src.WriteMask);
    }

    return rs;
}

GpuGraphicsPipeline NvrhiGpuDevice::CreateGraphicsPipeline(
    const GraphicsPipelineDesc& desc, GpuFramebuffer framebuffer)
{
    nvrhi::GraphicsPipelineDesc pd;
    pd.VS          = m_Shaders.Get(desc.VS.id);
    pd.PS          = m_Shaders.Get(desc.PS.id);
    if (desc.HS.IsValid()) pd.HS = m_Shaders.Get(desc.HS.id);
    if (desc.DS.IsValid()) pd.DS = m_Shaders.Get(desc.DS.id);
    if (desc.GS.IsValid()) pd.GS = m_Shaders.Get(desc.GS.id);
    pd.inputLayout = m_InputLayouts.Get(desc.InputLayout.id);
    pd.primType    = ToNvrhiPrimType(desc.PrimType);
    pd.renderState = ToNvrhiRenderState(desc);

    for (const auto& bl : desc.BindingLayouts)
        pd.bindingLayouts.push_back(m_BindingLayouts.Get(bl.id));

    auto nvFb = m_Framebuffers.Get(framebuffer.id);
    return { m_GraphicsPipelines.Add(m_Device->createGraphicsPipeline(pd, nvFb)) };
}

void NvrhiGpuDevice::DestroyGraphicsPipeline(GpuGraphicsPipeline handle)
{
    m_GraphicsPipelines.Release(handle.id);
}

GpuComputePipeline NvrhiGpuDevice::CreateComputePipeline(
    const ComputePipelineDesc& desc)
{
    nvrhi::ComputePipelineDesc pd;
    pd.CS = m_Shaders.Get(desc.CS.id);
    for (const auto& bl : desc.BindingLayouts)
        pd.bindingLayouts.push_back(m_BindingLayouts.Get(bl.id));

    return { m_ComputePipelines.Add(m_Device->createComputePipeline(pd)) };
}

void NvrhiGpuDevice::DestroyComputePipeline(GpuComputePipeline handle)
{
    m_ComputePipelines.Release(handle.id);
}

// ---- Command context ----

std::unique_ptr<ICommandContext> NvrhiGpuDevice::CreateCommandContext()
{
    auto cmdList = m_Device->createCommandList();
    return std::make_unique<NvrhiCommandContext>(this, std::move(cmdList));
}

void NvrhiGpuDevice::ExecuteCommandContext(ICommandContext& ctx)
{
    auto* nvrhi_ctx = static_cast<NvrhiCommandContext*>(&ctx);
    m_Device->executeCommandList(nvrhi_ctx->GetNative());
}

// ---- Device management ----

void NvrhiGpuDevice::WaitForIdle()
{
    m_Device->waitForIdle();
}

void NvrhiGpuDevice::RunGarbageCollection()
{
    m_Device->runGarbageCollection();
}

// ---- Back buffer registration (called by IRenderDevice backends) ----

void NvrhiGpuDevice::RegisterBackBuffers(
    const std::vector<nvrhi::TextureHandle>& nativeBuffers)
{
    m_BackBufferTextures.clear();
    m_BackBufferTextures.reserve(nativeBuffers.size());
    for (const auto& nvTex : nativeBuffers)
        m_BackBufferTextures.push_back({ m_Textures.Add(nvTex) });
}

void NvrhiGpuDevice::ClearBackBuffers()
{
    for (auto& t : m_BackBufferTextures)
        m_Textures.Release(t.id);
    m_BackBufferTextures.clear();
}

const std::vector<GpuTexture>& NvrhiGpuDevice::GetBackBufferTextures() const
{
    return m_BackBufferTextures;
}
