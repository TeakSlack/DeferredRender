#ifndef SKY_MANAGER_H
#define SKY_MANAGER_H

#include <Engine.h>
#include <Render/Sky/TransmittanceLUT.h>
#include <Render/Sky/MultiScatteringLUT.h>

class SkyManager : public IEngineSubmodule
{
public:
	// Sky is dirty by default to compute LUTs on first pass
	SkyManager() 
		: IEngineSubmodule("SkyManager"), m_IsDirty(true), m_Atmosphere(Atmosphere{}), m_Backend(RenderBackend::None) {}

	void Init() override;
	void Tick(float deltaTime) override;
	void Shutdown() override;

	void AddSkyPasses(RGMutableTextureHandle clearPass);

	// Must be called before first invocation of Tick
	void SetDevice(IGpuDevice* device);
	void SetBackend(RenderBackend backend);
	void SetFrameGraph(FrameGraph* fg);
	void SetCommandContext(ICommandContext* cmd);

private:
	IGpuDevice* m_Device;
	bool m_IsDirty;

	RenderBackend m_Backend;
	FrameGraph* m_FG = nullptr; // Non-owning, owned by client code
	ICommandContext* m_Cmd = nullptr; // Non-owning
	Atmosphere m_Atmosphere;
	TransmittanceLUT m_TransmittanceLUT;
	MultiScatteringLUT m_MultiScatteringLUT;
	GpuSampler m_LinearSampler;
};

#endif // SKY_MANAGER_H