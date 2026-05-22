#include "AppLayer.h"
#define NOMINMAX

#ifdef COMPILE_WITH_VULKAN
#include <Graphics/VulkanDevice.h>
#endif
#ifdef COMPILE_WITH_DX12
#include <Graphics/D3D12Device.h>
#endif

#include <Engine.h>
#include <Window/GLFWWindow.h>
#include <Events/ApplicationEvents.h>
#include <Render/GpuTypes.h>
#include <Render/BinaryLoader.h>

// -------------------------------------------------------------------------
// Framebuffer management
// -------------------------------------------------------------------------

void AppLayer::CreateFramebuffers()
{
	auto extent = m_WindowSystem->GetExtent(m_WindowHandle);
	m_Width  = (uint32_t)extent.x;
	m_Height = (uint32_t)extent.y;

	const auto& backBuffers = m_GpuDevice->GetBackBufferTextures();
	m_Framebuffers.reserve(backBuffers.size());
	for (GpuTexture colorTex : backBuffers)
	{
		FramebufferDesc fbDesc;
		fbDesc.ColorAttachments.push_back({ colorTex });
		m_Framebuffers.push_back(m_GpuDevice->CreateFramebuffer(fbDesc));
	}
}

void AppLayer::DestroyFramebuffers()
{
	for (auto fb : m_Framebuffers)
		m_GpuDevice->DestroyFramebuffer(fb);
	m_Framebuffers.clear();
}

// -------------------------------------------------------------------------
// Blit pipeline
// -------------------------------------------------------------------------

void AppLayer::CreateBlitPipeline()
{
	// ---- Sampler (created first — MultiScatteringLUT::Init needs it) ----
	SamplerDesc samplerDesc;
	samplerDesc.MinFilter = Filter::Linear;
	samplerDesc.MagFilter = Filter::Linear;
	samplerDesc.AddressU  = AddressMode::Clamp;
	samplerDesc.AddressV  = AddressMode::Clamp;
	m_LinearSampler = m_GpuDevice->CreateSampler(samplerDesc);

	// ---- Transmittance LUT (needed as input to MultiScatteringLUT) ----
	TransmittanceLUT::Config transmittanceConfig;
	transmittanceConfig.Backend    = RenderBackend::D3D12;
	transmittanceConfig.Atmosphere = Atmosphere{};
	m_TransmittanceLUT.Init(transmittanceConfig, m_GpuDevice, m_FrameGraph.get());
	m_TransmittanceLUT.Compute(m_CommandContext.get());

	// ---- Multi-scattering LUT ----
	MultiScatteringLUT::Config multiScatterConfig;
	multiScatterConfig.Backend    = RenderBackend::D3D12;
	multiScatterConfig.Atmosphere = Atmosphere{};
	m_MultiScatteringLUT.Init(multiScatterConfig,
	                          m_TransmittanceLUT.LUT(), m_LinearSampler,
	                          m_GpuDevice, m_FrameGraph.get());
	m_MultiScatteringLUT.Compute(m_CommandContext.get());

	// ---- Blit shaders ----
	auto vsBytecode = BinaryLoader::LoadBinary("shaders/blit.vs.cso");
	auto psBytecode = BinaryLoader::LoadBinary("shaders/blit.ps.cso");

	ShaderDesc vsDesc;
	vsDesc.Stage      = ShaderStage::Vertex;
	vsDesc.EntryPoint = "main";
	vsDesc.Bytecode   = vsBytecode.data();
	vsDesc.ByteSize   = vsBytecode.size();
	vsDesc.DebugName  = "Blit VS";
	GpuShader vsShader = m_GpuDevice->CreateShader(vsDesc);

	ShaderDesc psDesc;
	psDesc.Stage      = ShaderStage::Pixel;
	psDesc.EntryPoint = "main";
	psDesc.Bytecode   = psBytecode.data();
	psDesc.ByteSize   = psBytecode.size();
	psDesc.DebugName  = "Blit PS";
	GpuShader psShader = m_GpuDevice->CreateShader(psDesc);

	// ---- Binding layout: t0 = LUT texture, s0 = sampler ----
	BindingLayoutDesc layoutDesc;
	layoutDesc.Items = {
		BindingLayoutItem::Texture(0, ShaderStage::Pixel),
		BindingLayoutItem::Sampler(0, ShaderStage::Pixel)
	};
	m_BlitLayout = m_GpuDevice->CreateBindingLayout(layoutDesc);

	// ---- Binding set: blit the multi-scattering LUT output ----
	BindingSetDesc setDesc;
	setDesc.Items = {
		BindingItem::Texture(0, m_MultiScatteringLUT.LUT()),
		BindingItem::Sampler(0, m_LinearSampler)
	};
	m_BlitBindingSet = m_GpuDevice->CreateBindingSet(setDesc, m_BlitLayout);

	// ---- Graphics pipeline (fullscreen triangle, no vertex buffer) ----
	GraphicsPipelineDesc pipelineDesc;
	pipelineDesc.VS                  = vsShader;
	pipelineDesc.PS                  = psShader;
	pipelineDesc.PrimType            = PrimitiveType::TriangleList;
	pipelineDesc.BindingLayouts      = { m_BlitLayout };
	pipelineDesc.Rasterizer.CullMode = CullMode::None;

	m_BlitPipeline = m_GpuDevice->CreateGraphicsPipeline(pipelineDesc, m_Framebuffers[0]);

	m_GpuDevice->DestroyShader(vsShader);
	m_GpuDevice->DestroyShader(psShader);
}

void AppLayer::DestroyBlitPipeline()
{
	m_MultiScatteringLUT.Destroy();
	m_TransmittanceLUT.Destroy();
	m_GpuDevice->DestroyGraphicsPipeline(m_BlitPipeline);
	m_GpuDevice->DestroyBindingSet(m_BlitBindingSet);
	m_GpuDevice->DestroyBindingLayout(m_BlitLayout);
	m_GpuDevice->DestroySampler(m_LinearSampler);
}

// -------------------------------------------------------------------------
// Layer lifecycle
// -------------------------------------------------------------------------

void AppLayer::OnAttach()
{
	WindowDesc desc;
	desc.title  = "TestBed";
	desc.width  = 1280;
	desc.height = 720;
	m_WindowSystem = Engine::Get().GetSubmodule<GLFWWindowSystem>();
	m_WindowHandle = m_WindowSystem->OpenWindow(desc);

	auto* glfwWS = dynamic_cast<GLFWWindowSystem*>(m_WindowSystem);
	m_GlfwWindow  = glfwWS->GetGLFWWindow(m_WindowHandle);

	m_RenderDevice = std::make_unique<D3D12Device>(glfwWS->GetNativeHandle(m_WindowHandle));
	m_GpuDevice    = m_RenderDevice->CreateDevice();

	auto extent = m_WindowSystem->GetExtent(m_WindowHandle);
	m_RenderDevice->CreateSwapchain((uint32_t)extent.x, (uint32_t)extent.y);

	m_CommandContext = m_GpuDevice->CreateCommandContext();
	m_FrameGraph     = std::make_unique<FrameGraph>();

	CreateFramebuffers();
	CreateBlitPipeline();
}

void AppLayer::OnDetach()
{
	if (m_GpuDevice)
		m_GpuDevice->WaitForIdle();

	DestroyBlitPipeline();
	DestroyFramebuffers();

	m_CommandContext.reset();
	m_FrameGraph.reset();
	m_GpuDevice    = nullptr;
	m_RenderDevice.reset();

	m_WindowSystem->CloseWindow(m_WindowHandle);
}

void AppLayer::OnUpdate(float /*deltaTime*/)
{
	if (m_WindowSystem->ShouldClose(m_WindowHandle))
	{
		Engine::Get().RequestStop();
		return;
	}

	if (m_PendingResize)
	{
		m_GpuDevice->WaitForIdle();
		m_GpuDevice->RunGarbageCollection();
		DestroyFramebuffers();
		m_RenderDevice->RecreateSwapchain(m_Width, m_Height);
		CreateFramebuffers();
		m_PendingResize = false;
	}

	m_RenderDevice->BeginFrame();
	m_GpuDevice->RunGarbageCollection();

	uint32_t imageIdx = m_RenderDevice->GetCurrentImageIndex();
	GpuFramebuffer fb = m_Framebuffers[imageIdx];

	// ---- FrameGraph ----
	m_FrameGraph->Reset();

	TextureDesc bbDesc;
	bbDesc.Width     = m_Width;
	bbDesc.Height    = m_Height;
	bbDesc.Format    = GpuFormat::BGRA8_UNORM;
	bbDesc.DebugName = "Backbuffer";
	RGMutableTextureHandle backbuffer = m_FrameGraph->ImportMutableTexture(
		m_GpuDevice->GetBackBufferTextures()[imageIdx],
		bbDesc,
		m_FrameCount++ == 0 ? ResourceLayout::Undefined : ResourceLayout::Present);

	// ---- Clear pass ----
	struct ClearPassData { RGMutableTextureHandle target; };
	auto& clearPass = m_FrameGraph->AddCallbackPass<ClearPassData>(
		"Clear",
		[&](PassBuilder& builder, ClearPassData& data)
		{
			data.target = builder.WriteTexture(backbuffer);
		},
		[fb](const ClearPassData&, const RenderPassResources&, ICommandContext* cmd)
		{
			RenderPassDesc passDesc;
			passDesc.Framebuffer = fb;
			passDesc.ClearColor  = true;
			passDesc.ColorValue  = ClearValue{ 1.0f, 0.0f, 0.0f, 1.0f };
			passDesc.ClearDepth  = false;
			cmd->BeginRenderPass(passDesc);
			cmd->EndRenderPass();
		}
	);

	// ---- Blit pass: draw MultiScatteringLUT to backbuffer ----
	struct BlitPassData { RGMutableTextureHandle target; };
	m_FrameGraph->AddCallbackPass<BlitPassData>(
		"BlitMultiScatteringLUT",
		[&](PassBuilder& builder, BlitPassData& data)
		{
			data.target = builder.WriteTexture(clearPass.data.target);
		},
		[fb, this](const BlitPassData&, const RenderPassResources&, ICommandContext* cmd)
		{
			RenderPassDesc passDesc;
			passDesc.Framebuffer = fb;
			passDesc.ClearColor  = false;
			cmd->BeginRenderPass(passDesc);

			cmd->SetGraphicsPipeline(m_BlitPipeline);
			cmd->SetBindingSet(m_BlitBindingSet);
			cmd->SetViewport(0.0f, 0.0f, (float)m_Width, (float)m_Height);
			cmd->SetScissor(0, 0, m_Width, m_Height);

			DrawArgs args;
			args.VertexCount   = 3;
			args.InstanceCount = 1;
			cmd->Draw(args);

			cmd->EndRenderPass();
		}
	);

	m_FrameGraph->Compile();

	m_CommandContext->Open();
	m_FrameGraph->Execute(m_GpuDevice, m_CommandContext.get());
	m_CommandContext->Close();

	m_GpuDevice->ExecuteCommandContext(*m_CommandContext);
	m_RenderDevice->Present();
}

void AppLayer::OnEvent(Event& event)
{
	EventDispatcher d(event);

	d.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e)
	{
		if (e.GetWindow() == m_WindowHandle)
		{
			m_Width        = e.GetWidth();
			m_Height       = e.GetHeight();
			m_PendingResize = true;
		}
		return false;
	});
}
