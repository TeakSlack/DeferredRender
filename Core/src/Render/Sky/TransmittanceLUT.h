#ifndef TRANSMITTANCE_LUT_H
#define TRANSMITTANCE_LUT_H

#include <Render/FrameGraph/FrameGraph.h>
#include <Render/Sky/Atmosphere.h>
#include <Render/BinaryLoader.h>
#include <Graphics/RenderBackend.h>

// -------------------------------------------------------------------------
// TransmittanceLUT - precomputes a 2D lookup table for atmospheric transmittance
// -------------------------------------------------------------------------
class TransmittanceLUT
{
public:
	struct Config
	{
		Atmosphere    Atmosphere;
		RenderBackend Backend;
		uint32_t      Width  = 256;
		uint32_t      Height = 64;
	};

	void Init(const Config& config, IGpuDevice* device, FrameGraph* fg)
	{
		m_Backend    = config.Backend;
		m_Atmosphere = config.Atmosphere;
		m_Width      = config.Width;
		m_Height     = config.Height;
		m_Device     = device;
		m_FG         = fg;

		BufferDesc dataDesc;
		dataDesc.ByteSize = sizeof(TransmittanceLUTData);
		dataDesc.Usage    = BufferUsage::Constant;
		dataDesc.DebugName = "TransmittanceLUTData";
		m_ConstantBuffer = device->CreateBuffer(dataDesc);

		// ---- Transmittance LUT texture (2D) ----
		TextureDesc texDesc;
		texDesc.Format    = GpuFormat::RGBA16_FLOAT;
		texDesc.DebugName = "TransmittanceLUT";
		texDesc.Width     = m_Width;
		texDesc.Height    = m_Height;
		texDesc.Usage     = TextureUsage::ShaderResource | TextureUsage::Storage;
		m_TransmittanceLUT = device->CreateTexture(texDesc);

		// ---- Compute shader ----
		const bool vulkan    = m_Backend == RenderBackend::Vulkan;
		const char* vkPath   = "shaders/TransmittanceLUT.cs.spv";
		const char* dxilPath = "shaders/TransmittanceLUT.cs.cso";
		std::vector<uint8_t> byteCode = BinaryLoader::LoadBinary(vulkan ? vkPath : dxilPath);

		ShaderDesc shaderDesc;
		shaderDesc.Bytecode   = byteCode.data();
		shaderDesc.ByteSize   = byteCode.size();
		shaderDesc.EntryPoint = "Main";
		shaderDesc.Stage      = ShaderStage::Compute;
		shaderDesc.DebugName  = "TransmittanceLUT Compute Shader";
		m_Shader = device->CreateShader(shaderDesc);

		// ---- Binding layout ----
		BindingLayoutDesc layoutDesc;
		layoutDesc.Items = {
			BindingLayoutItem::ConstantBuffer(0, ShaderStage::Compute),
			BindingLayoutItem::StorageTexture(0, ShaderStage::Compute)
		};
		m_Layout = device->CreateBindingLayout(layoutDesc);

		// ---- Binding set ----
		BindingSetDesc setDesc;
		setDesc.Items = {
			BindingItem::ConstantBuffer(0, m_ConstantBuffer),
			BindingItem::StorageTexture(0, m_TransmittanceLUT)
		};
		m_BindingSet = device->CreateBindingSet(setDesc, m_Layout);

		// ---- Compute pipeline ----
		ComputePipelineDesc pipelineDesc;
		pipelineDesc.CS             = m_Shader;
		pipelineDesc.BindingLayouts = { m_Layout };
		m_Pipeline = device->CreateComputePipeline(pipelineDesc);
	}

	void Destroy()
	{
		m_Device->DestroyComputePipeline(m_Pipeline);
		m_Device->DestroyBindingSet(m_BindingSet);
		m_Device->DestroyBindingLayout(m_Layout);
		m_Device->DestroyShader(m_Shader);
		m_Device->DestroyTexture(m_TransmittanceLUT);
		m_Device->DestroyBuffer(m_ConstantBuffer);
	}

	RGMutableTextureHandle Compute(ICommandContext* cmd)
	{
		TransmittanceLUTData data = BuildGpuData(m_Atmosphere, m_Width, m_Height);

		cmd->Open();
		cmd->WriteBuffer(m_ConstantBuffer, &data, sizeof(TransmittanceLUTData));
		cmd->SetComputePipeline(m_Pipeline);
		cmd->SetComputeBindingSet(m_BindingSet);
		DispatchArgs args;
		args.GroupX = (m_Width  + k_ThreadGroupX - 1) / k_ThreadGroupX;
		args.GroupY = (m_Height + k_ThreadGroupY - 1) / k_ThreadGroupY;
		args.GroupZ = 1;
		cmd->Dispatch(args);
		cmd->TransitionTexture(m_TransmittanceLUT, ResourceLayout::ShaderResource);
		cmd->Close();

		m_Device->ExecuteCommandContext(*cmd);
		m_Device->WaitForIdle();

		TextureDesc desc;
		desc.Width     = m_Width;
		desc.Height    = m_Height;
		desc.Format    = GpuFormat::RGBA16_FLOAT;
		desc.DebugName = "TransmittanceLUT";
		desc.Usage     = TextureUsage::Storage | TextureUsage::ShaderResource;
		return m_FG->ImportMutableTexture(m_TransmittanceLUT, desc, ResourceLayout::ShaderResource);
	}

	GpuTexture LUT() { return m_TransmittanceLUT; }

private:
	static constexpr uint32_t k_ThreadGroupX = 32; // must match [numthreads(X, ...)] in TransmittanceLUT.cs
	static constexpr uint32_t k_ThreadGroupY = 8;  // must match [numthreads(..., Y, ...)] in TransmittanceLUT.cs

	RenderBackend      m_Backend;
	IGpuDevice*        m_Device    = nullptr;
	FrameGraph*        m_FG        = nullptr;
	Atmosphere         m_Atmosphere;
	uint32_t           m_Width     = 256;
	uint32_t           m_Height    = 64;
	GpuBuffer          m_ConstantBuffer;
	GpuShader          m_Shader;
	GpuBindingLayout   m_Layout;
	GpuBindingSet      m_BindingSet;
	GpuComputePipeline m_Pipeline;
	GpuTexture         m_TransmittanceLUT;

	struct alignas(16) TransmittanceLUTData
	{
		uint32_t Width;               // offset   0
		uint32_t Height;              // offset   4
		float    BottomRadius;        // offset   8
		float    TopRadius;           // offset  12

		Vector3  Center;              // offset  16
		float    _pad0;               // offset  28

		Vector3  RayleighScattering;  // offset  32
		float    RayleighDensityExpScale; // offset 44

		Vector3  MieScattering;       // offset  48
		float    MieDensityExpScale;  // offset  60
		Vector3  MieExtinction;       // offset  64
		float    MiePhaseParam;       // offset  76

		float    LayerHeight;         // offset  80
		float    LinearTerm;          // offset  84
		float    ConstantTerm;        // offset  88
		float    LinearTerm1;         // offset  92
		float    ConstantTerm1;       // offset  96

		Vector3  AbsorptionExtinction;    // offset 100
		float    MultipleScatteringFactor; // offset 112
		Vector3  GroundAlbedo;        // offset 116
	};
	static_assert(sizeof(TransmittanceLUTData) % 16 == 0, "TransmittanceLUTData must be 16-byte aligned for GPU upload");

	static TransmittanceLUTData BuildGpuData(const Atmosphere& atm, uint32_t width, uint32_t height)
	{
		TransmittanceLUTData data;
		data.Width                    = width;
		data.Height                   = height;
		data.BottomRadius             = atm.BottomRadius;
		data.TopRadius                = atm.TopRadius;
		data.Center                   = atm.Center;
		data._pad0                    = 0.0f;
		data.RayleighScattering       = atm.RayleighScattering;
		data.RayleighDensityExpScale  = atm.RayleighDensityExpScale;
		data.MieScattering            = atm.MieScattering;
		data.MieDensityExpScale       = atm.MieDensityExpScale;
		data.MieExtinction            = atm.MieExtinction;
		data.MiePhaseParam            = atm.MiePhaseParam;
		data.LayerHeight              = atm.LayerHeight;
		data.LinearTerm               = atm.LinearTerm;
		data.ConstantTerm             = atm.ConstantTerm;
		data.LinearTerm1              = atm.LinearTerm1;
		data.ConstantTerm1            = atm.ConstantTerm1;
		data.AbsorptionExtinction     = atm.AbsorptionExtinction;
		data.MultipleScatteringFactor = atm.MultipleScatteringFactor;
		data.GroundAlbedo             = atm.GroundAlbedo;
		return data;
	}
};

#endif // TRANSMITTANCE_LUT_H
