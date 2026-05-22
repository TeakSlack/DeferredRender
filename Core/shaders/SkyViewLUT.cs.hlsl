cbuffer SkyViewLUTData : register(b0)
{
    uint   Width;
    uint   Height;
    float  BottomRadius;
    float  TopRadius;

    float3 Center;
    float  _pad0;

    float3 RayleighScattering;
    float  RayleighDensityExpScale;

    float3 MieScattering;
    float  MieDensityExpScale;
    float3 MieExtinction;
    float  MiePhaseParam;

    float  LayerHeight;
    float  LinearTerm;
    float  ConstantTerm;
    float  LinearTerm1;
    float  ConstantTerm1;

    float3 AbsorptionExtinction;
    float  MultipleScatteringFactor;
    float3 GroundAlbedo;

    float3 CameraWorldPos;
    float  _pad1;

    float3 SunDirection;
    float  _pad2;

    float3 SunIlluminance;
    float  _pad3;

    float3 MoonDirection;
    float  _pad4;

    float3 MoonIlluminance;
    float  _pad5;
}

Texture2D<float4>    TransmittanceLUT    : register(t0);
Texture2D<float4>    MultiScatteringLUT  : register(t1);
SamplerState         LinearSampler       : register(s0);
RWTexture2D<float4>  LUT                 : register(u0);

// ---------------------------------------------------------------------------

static const float pi  = 3.14159265358979323846;
static const float tau = pi * 2.0;

// ---------------------------------------------------------------------------
// Atmosphere helpers

struct Atmosphere
{
    float  bottom_radius;
    float  top_radius;
    float3 center;
    float3 rayleigh_scattering;
    float  rayleigh_density_exp_scale;
    float3 mie_scattering;
    float  mie_density_exp_scale;
    float3 mie_extinction;
    float  mie_phase_param;
    float  absorption_density_0_layer_height;
    float  absorption_density_0_linear_term;
    float  absorption_density_0_constant_term;
    float  absorption_density_1_linear_term;
    float  absorption_density_1_constant_term;
    float3 absorption_extinction;
    float  multiple_scattering_factor;
    float3 ground_albedo;
};

Atmosphere GetAtmosphere()
{
    Atmosphere atm;
    atm.bottom_radius                      = BottomRadius;
    atm.top_radius                         = TopRadius;
    atm.center                             = Center;
    atm.rayleigh_scattering                = RayleighScattering;
    atm.rayleigh_density_exp_scale         = RayleighDensityExpScale;
    atm.mie_scattering                     = MieScattering;
    atm.mie_density_exp_scale              = MieDensityExpScale;
    atm.mie_extinction                     = MieExtinction;
    atm.mie_phase_param                    = MiePhaseParam;
    atm.absorption_density_0_layer_height  = LayerHeight;
    atm.absorption_density_0_linear_term   = LinearTerm;
    atm.absorption_density_0_constant_term = ConstantTerm;
    atm.absorption_density_1_linear_term   = LinearTerm1;
    atm.absorption_density_1_constant_term = ConstantTerm1;
    atm.absorption_extinction              = AbsorptionExtinction;
    atm.multiple_scattering_factor         = MultipleScatteringFactor;
    atm.ground_albedo                      = GroundAlbedo;
    return atm;
}

// ---------------------------------------------------------------------------
// Sky-view LUT parametrization
//
// UV -> (azimuth, elevation) following Hillaire 2020:
//   u -> azimuth  [0, 2*pi]
//   v -> elevation mapped non-linearly around the horizon

float2 UvToSkyViewLutParams(float2 uv, float viewHeight, Atmosphere atm)
{
    float vHorizon   = sqrt(max(0.0, viewHeight * viewHeight - atm.bottom_radius * atm.bottom_radius));
    float cosBeta    = vHorizon / viewHeight;
    float beta       = acos(cosBeta);
    float zenithHorizonAngle = pi - beta;

    float viewZenithAngle;
    if (uv.y < 0.5)
    {
        float coord = 1.0 - 2.0 * uv.y;
        coord = 1.0 - coord * coord;
        viewZenithAngle = zenithHorizonAngle * coord;
    }
    else
    {
        float coord = 2.0 * uv.y - 1.0;
        coord = coord * coord;
        viewZenithAngle = zenithHorizonAngle + beta * coord;
    }

    float azimuth = uv.x * tau;
    return float2(viewZenithAngle, azimuth);
}

// ---------------------------------------------------------------------------

[numthreads(16, 16, 1)]
void Main(uint3 dispatchId : SV_DispatchThreadID)
{
    if (dispatchId.x >= Width || dispatchId.y >= Height)
        return;

    float2 uv = (dispatchId.xy + 0.5) / float2(Width, Height);

    // TODO: raymarch atmosphere along view direction, accumulating
    //       in-scattering from both sun and moon using TransmittanceLUT
    //       and MultiScatteringLUT.

    LUT[dispatchId.xy] = float4(0.0, 0.0, 0.0, 1.0);
}
