cbuffer TransmittanceLUTData : register(b0)
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
};

RWTexture2D<float4> LUT : register(u0);

// ---------------------------------------------------------------------------
// Ray / circle helpers

float SolveQuadratic(float a, float b, float c)
{
    float delta = b * b - 4.0 * a * c;
    if (delta < 0.0 || a == 0.0)
        return -1.0;
    float solution0 = (-b - sqrt(delta)) / (2.0 * a);
    float solution1 = (-b + sqrt(delta)) / (2.0 * a);
    if (solution0 < 0.0 && solution1 < 0.0)
        return -1.0;
    if (solution0 < 0.0)
        return max(0.0, solution1);
    if (solution1 < 0.0)
        return max(0.0, solution0);
    return max(0.0, min(solution0, solution1));
}

float FindClosestRayCircleIntersection(float2 o, float2 d, float r)
{
    return SolveQuadratic(dot(d, d), 2.0 * dot(d, o), dot(o, o) - r * r);
}

// Returns the distance along d to where the ray exits the atmosphere.
// If the ray hits the ground first, t_max is clamped to the ground intersection.
bool FindAtmosphereTMax2D(out float tMax, float2 o, float2 d, float bottomRadius, float topRadius)
{
    float tTop    = FindClosestRayCircleIntersection(o, d, topRadius);
    float tBottom = FindClosestRayCircleIntersection(o, d, bottomRadius);

    if (tTop <= 0.0)
    {
        tMax = 0.0;
        return false;
    }

    tMax = tTop;
    if (tBottom > 0.0)
        tMax = min(tTop, tBottom);

    return true;
}

// ---------------------------------------------------------------------------
// Atmosphere struct + cbuffer bridge

static const float t_max_max = 9000000.0;

struct Atmosphere
{
    float  bottom_radius;
    float  top_radius;
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
    float3 ground_albedo;
    float  multiple_scattering_factor;
};

Atmosphere MakeAtmosphere()
{
    Atmosphere a;
    a.bottom_radius                      = BottomRadius;
    a.top_radius                         = TopRadius;
    a.rayleigh_scattering                = RayleighScattering;
    a.rayleigh_density_exp_scale         = RayleighDensityExpScale;
    a.mie_scattering                     = MieScattering;
    a.mie_density_exp_scale              = MieDensityExpScale;
    a.mie_extinction                     = MieExtinction;
    a.mie_phase_param                    = MiePhaseParam;
    a.absorption_density_0_layer_height  = LayerHeight;
    a.absorption_density_0_linear_term   = LinearTerm;
    a.absorption_density_0_constant_term = ConstantTerm;
    a.absorption_density_1_linear_term   = LinearTerm1;
    a.absorption_density_1_constant_term = ConstantTerm1;
    a.absorption_extinction              = AbsorptionExtinction;
    a.ground_albedo                      = GroundAlbedo;
    a.multiple_scattering_factor         = MultipleScatteringFactor;
    return a;
}

// ---------------------------------------------------------------------------
// Medium

float3 SampleMediumExtinction(float height, Atmosphere atmosphere)
{
    float mie_density      = exp(atmosphere.mie_density_exp_scale * height);
    float rayleigh_density = exp(atmosphere.rayleigh_density_exp_scale * height);

    float absorption_density;
    if (height < atmosphere.absorption_density_0_layer_height)
        absorption_density = saturate(atmosphere.absorption_density_0_linear_term * height +
            atmosphere.absorption_density_0_constant_term);
    else
        absorption_density = saturate(atmosphere.absorption_density_1_linear_term * height +
            atmosphere.absorption_density_1_constant_term);

    float3 mie_extinction        = mie_density       * atmosphere.mie_extinction;
    float3 rayleigh_extinction   = rayleigh_density   * atmosphere.rayleigh_scattering;
    float3 absorption_extinction = absorption_density * atmosphere.absorption_extinction;

    return mie_extinction + rayleigh_extinction + absorption_extinction;
}

// ---------------------------------------------------------------------------
// LUT parameterization

float2 UVToTransmittanceLUTParams(float2 uv, Atmosphere atmosphere)
{
    float x_mu = uv.x;
    float x_r  = uv.y;

    float bottom_radius_sq = atmosphere.bottom_radius * atmosphere.bottom_radius;
    float h_sq             = atmosphere.top_radius * atmosphere.top_radius - bottom_radius_sq;
    float h                = sqrt(h_sq);
    float rho              = h * x_r;
    float rho_sq           = rho * rho;
    float view_height      = sqrt(rho_sq + bottom_radius_sq);

    float d_min = atmosphere.top_radius - view_height;
    float d_max = rho + h;
    float d     = d_min + x_mu * (d_max - d_min);

    float cos_view_zenith = 1.0;
    if (d != 0.0)
        cos_view_zenith = clamp((h_sq - rho_sq - d * d) / (2.0 * view_height * d), -1.0, 1.0);

    return float2(view_height, cos_view_zenith);
}

// ---------------------------------------------------------------------------

#ifndef SAMPLE_COUNT
#define SAMPLE_COUNT 40
#endif

[numthreads(32, 8, 1)]
void Main(uint3 globalID : SV_DispatchThreadID)
{
    if (globalID.x >= Width || globalID.y >= Height)
        return;

    float2 uv = (float2(globalID.xy) + 0.5) / float2(Width, Height);

    Atmosphere atmosphere = MakeAtmosphere();

    float2 lut_params      = UVToTransmittanceLUTParams(uv, atmosphere);
    float  view_height     = lut_params.x;
    float  cos_view_zenith = lut_params.y;

    float2 world_pos = float2(0.0, view_height);
    float2 world_dir = float2(sqrt(max(1.0 - cos_view_zenith * cos_view_zenith, 0.0)), cos_view_zenith);

    float3 transmittance = float3(0.0, 0.0, 0.0);

    float tMax;
    if (FindAtmosphereTMax2D(tMax, world_pos, world_dir, atmosphere.bottom_radius, atmosphere.top_radius))
    {
        tMax = min(tMax, t_max_max);

        float sample_count     = float(SAMPLE_COUNT);
        float sample_segment_t = 0.3;
        float dt               = tMax / sample_count;

        float t        = 0.0;
        float dt_exact = 0.0;
        for (float s = 0.0; s < sample_count; s += 1.0)
        {
            float t_new = (s + sample_segment_t) * dt;
            dt_exact    = t_new - t;
            t           = t_new;

            float sample_height = length(world_pos + t * world_dir) - atmosphere.bottom_radius;
            transmittance += SampleMediumExtinction(sample_height, atmosphere) * dt_exact;
        }

        transmittance = exp(-transmittance);
    }

    LUT[globalID.xy] = float4(transmittance, 1.0);
}
