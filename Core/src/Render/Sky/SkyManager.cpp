#include "SkyManager.h"

void SkyManager::Init()
{
	SamplerDesc desc;
	desc.AddressU = AddressMode::Clamp;
	desc.AddressV = AddressMode::Clamp;
	desc.MinFilter = Filter::Linear;
	desc.MagFilter = Filter::Linear;
	m_LinearSampler = m_Device->CreateSampler(desc);
}

void SkyManager::Tick(float deltaTime)
{
	if (!m_IsDirty)
		return;

	m_IsDirty = false;

	TransmittanceLUT::Config transmittanceConfig{};
	transmittanceConfig.Backend = m_Backend;
	transmittanceConfig.Atmosphere = m_Atmosphere;
	m_TransmittanceLUT.Init(transmittanceConfig, m_Device, m_FG);
	m_TransmittanceLUT.Compute(m_Cmd);

	MultiScatteringLUT::Config multiScatterConfig;
	multiScatterConfig.Backend = m_Backend;
	multiScatterConfig.Atmosphere = m_Atmosphere;
	m_MultiScatteringLUT.Init(multiScatterConfig,
		m_TransmittanceLUT.LUT(), m_LinearSampler,
		m_Device, m_FG);
	m_MultiScatteringLUT.Compute(m_Cmd);
}

void SkyManager::Shutdown()
{
	m_TransmittanceLUT.Destroy();
	m_MultiScatteringLUT.Destroy();
}

void SkyManager::AddSkyPasses(RGMutableTextureHandle clearPass)
{
	struct SkyPassData { RGMutableTextureHandle target; };

	m_FG->AddCallbackPass<SkyPassData>("SkyPass",
		[&](PassBuilder& builder, SkyPassData& data)
		{
			data.target = builder.WriteTexture(clearPass);
		},
		[](const SkyPassData&, const RenderPassResources&, ICommandContext* cmd)
		{

		});
}

void SkyManager::SetDevice(IGpuDevice* device)
{
	m_Device = device;
}

void SkyManager::SetBackend(RenderBackend backend)
{
	m_Backend = backend;
}

void SkyManager::SetFrameGraph(FrameGraph* fg)
{
	m_FG = fg;
}

void SkyManager::SetCommandContext(ICommandContext* cmd)
{
	m_Cmd = cmd;
}
