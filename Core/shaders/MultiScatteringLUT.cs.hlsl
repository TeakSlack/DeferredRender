cbuffer MultiScatteringLUTData : register(b0)
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

Texture2D<float4>    TransmittanceLUT   : register(t0);
SamplerState         LinearSampler      : register(s0);
RWTexture2D<float4>  LUT                : register(u0);

// ---------------------------------------------------------------------------

static const float  pi                  = 3.14159265358979323846;
static const float  tau                 = pi * 2.0;
static const float  goldenRatio         = 1.6180339887498948482;
static const float  tMaxMax             = 9000000.0;
static const float  planetRadiusOffset  = 0.01;
static const float  isotropicPhase      = 1.0 / (4.0 * pi);

#define WORKGROUP_SIZE_Z 64
static const float DIRECTION_SAMPLE_COUNT = float(WORKGROUP_SIZE_Z);

// ---------------------------------------------------------------------------
// Atmosphere struct + cbuffer bridge

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

struct MediumSample
{
    float3 scattering;
    float3 extinction;
    float3 mie_scattering;
    float3 rayleigh_scattering;
};

MediumSample SampleMedium(float height, Atmosphere atmosphere)
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

    MediumSample s;
    s.mie_scattering      = mie_density      * atmosphere.mie_scattering;
    s.rayleigh_scattering = rayleigh_density  * atmosphere.rayleigh_scattering;
    s.scattering          = s.mie_scattering + s.rayleigh_scattering;
    s.extinction          = mie_density      * atmosphere.mie_extinction
                          + s.rayleigh_scattering
                          + absorption_density * atmosphere.absorption_extinction;
    return s;
}

// ---------------------------------------------------------------------------
// Intersection

float SolveQuadraticForPositiveReals(float a, float b, float c)
{
    float delta = b * b - 4.0 * a * c;
    if (delta < 0.0 || a == 0.0) return -1.0;
    float s0 = (-b - sqrt(delta)) / (2.0 * a);
    float s1 = (-b + sqrt(delta)) / (2.0 * a);
    if (s0 < 0.0 && s1 < 0.0) return -1.0;
    if (s0 < 0.0) return max(0.0, s1);
    if (s1 < 0.0) return max(0.0, s0);
    return max(0.0, min(s0, s1));
}

float FindClosestRaySphereIntersection(float3 o, float3 d, float3 c, float r)
{
    float3 dist = o - c;
    return SolveQuadraticForPositiveReals(dot(d, d), 2.0 * dot(d, dist), dot(dist, dist) - r * r);
}

bool RayIntersectsSphere(float3 o, float3 d, float3 c, float r)
{
    float3 dist  = o - c;
    float  a     = dot(d, d);
    float  b     = 2.0 * dot(d, dist);
    float  cc    = dot(dist, dist) - r * r;
    float  delta = b * b - 4.0 * a * cc;
    return (delta >= 0.0 && a != 0.0) &&
           (((-b - sqrt(max(delta, 0.0))) / (2.0 * a)) >= 0.0 ||
            ((-b + sqrt(max(delta, 0.0))) / (2.0 * a)) >= 0.0);
}

float ComputePlanetShadow(float3 o, float3 d, float3 c, float r)
{
    return RayIntersectsSphere(o, d, c, r) ? 0.0 : 1.0;
}

bool FindAtmosphereTMaxTBottom(out float t_max, out float t_bottom,
                               float3 o, float3 d, float3 c,
                               float bottom_radius, float top_radius)
{
    t_bottom    = FindClosestRaySphereIntersection(o, d, c, bottom_radius);
    float t_top = FindClosestRaySphereIntersection(o, d, c, top_radius);
    if (t_bottom < 0.0)
    {
        if (t_top < 0.0) { t_max = 0.0; return false; }
        else             { t_max = t_top; }
    }
    else
    {
        t_max = (t_top > 0.0) ? min(t_top, t_bottom) : t_bottom;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Transmittance LUT lookup

float2 TransmittanceLUTParamsToUV(Atmosphere atmosphere, float view_height, float cos_view_zenith)
{
    float height_sq        = view_height * view_height;
    float bottom_radius_sq = atmosphere.bottom_radius * atmosphere.bottom_radius;
    float top_radius_sq    = atmosphere.top_radius * atmosphere.top_radius;
    float h   = sqrt(max(0.0, top_radius_sq - bottom_radius_sq));
    float rho = sqrt(max(0.0, height_sq - bottom_radius_sq));

    float discriminant     = height_sq * (cos_view_zenith * cos_view_zenith - 1.0) + top_radius_sq;
    float dist_to_boundary = max(0.0, -view_height * cos_view_zenith + sqrt(max(discriminant, 0.0)));

    float x_mu = (dist_to_boundary - (atmosphere.top_radius - view_height)) /
                 (rho + h - (atmosphere.top_radius - view_height));
    float x_r  = rho / h;
    return float2(x_mu, x_r);
}

float3 GetTransmittanceToSun(float3 sun_dir, float3 zenith, Atmosphere atmosphere, float sample_height)
{
    float  cos_sun_zenith = dot(sun_dir, zenith);
    float2 uv = TransmittanceLUTParamsToUV(atmosphere, sample_height, cos_sun_zenith);
    return TransmittanceLUT.SampleLevel(LinearSampler, uv, 0).rgb;
}

// ---------------------------------------------------------------------------
// UV helpers

float FromSubUVsToUnit(float u, float resolution)
{
    return (u - 0.5 / resolution) * (resolution / (resolution - 1.0));
}

// ---------------------------------------------------------------------------
// Integration

struct IntegrationResults
{
    float3 luminance;
    float3 multi_scattering;
};

#ifndef SAMPLE_COUNT
#define SAMPLE_COUNT 20
#endif

IntegrationResults IntegrateScatteredLuminance(float3 world_pos, float3 world_dir,
                                               float3 sun_dir, Atmosphere atmosphere)
{
    IntegrationResults result;
    result.luminance        = float3(0, 0, 0);
    result.multi_scattering = float3(0, 0, 0);

    const float3 planet_center = float3(0, 0, 0);

    float t_max, t_bottom;
    if (!FindAtmosphereTMaxTBottom(t_max, t_bottom, world_pos, world_dir, planet_center,
                                   atmosphere.bottom_radius, atmosphere.top_radius))
        return result;

    t_max = min(t_max, tMaxMax);

    const float sample_count     = float(SAMPLE_COUNT);
    const float sample_segment_t = 0.3;
    const float dt               = t_max / sample_count;

    float3 throughput = float3(1, 1, 1);
    float  t = 0.0, dt_exact = 0.0;
    for (float s = 0.0; s < sample_count; s += 1.0)
    {
        float  t_new = (s + sample_segment_t) * dt;
        dt_exact = t_new - t;
        t        = t_new;

        float3 sample_pos    = world_pos + t * world_dir;
        float  sample_height = length(sample_pos);
        float3 zenith        = sample_pos / sample_height;

        float3 transmittance_to_sun  = GetTransmittanceToSun(sun_dir, zenith, atmosphere, sample_height);
        MediumSample medium          = SampleMedium(sample_height - atmosphere.bottom_radius, atmosphere);
        float3 sample_transmittance  = exp(-medium.extinction * dt_exact);

        float  planet_shadow         = ComputePlanetShadow(sample_pos, sun_dir,
                                           planet_center + planetRadiusOffset * zenith,
                                           atmosphere.bottom_radius);
        float3 scattered_luminance   = planet_shadow * transmittance_to_sun *
                                       (medium.scattering * isotropicPhase);

        result.multi_scattering += throughput *
            (medium.scattering - medium.scattering * sample_transmittance) / medium.extinction;
        result.luminance        += throughput *
            (scattered_luminance - scattered_luminance * sample_transmittance) / medium.extinction;

        throughput *= sample_transmittance;
    }

    // Light bounced off the planet surface
    if (t_max == t_bottom && t_bottom > 0.0)
    {
        float3 sample_pos    = world_pos + t_bottom * world_dir;
        float  sample_height = length(sample_pos);
        float3 zenith        = sample_pos / sample_height;

        float3 transmittance_to_sun = GetTransmittanceToSun(sun_dir, zenith, atmosphere, sample_height);
        float  n_dot_l = saturate(dot(zenith, sun_dir));
        result.luminance += transmittance_to_sun * throughput * n_dot_l * atmosphere.ground_albedo / pi;
    }

    return result;
}

// ---------------------------------------------------------------------------
// Sample direction (Fibonacci sphere)

float3 ComputeSampleDirection(uint direction_index)
{
    float sample    = float(direction_index);
    float theta     = tau * sample / goldenRatio;
    float phi       = acos(1.0 - 2.0 * (sample + 0.5) / DIRECTION_SAMPLE_COUNT);
    float cos_phi   = cos(phi), sin_phi = sin(phi);
    float cos_theta = cos(theta), sin_theta = sin(theta);
    return float3(cos_theta * sin_phi, sin_theta * sin_phi, cos_phi);
}

// ---------------------------------------------------------------------------
// Workgroup shared memory

groupshared float3 shared_multi_scattering[WORKGROUP_SIZE_Z];
groupshared float3 shared_luminance[WORKGROUP_SIZE_Z];

// ---------------------------------------------------------------------------

[numthreads(1, 1, WORKGROUP_SIZE_Z)]
void Main(uint3 globalID : SV_DispatchThreadID)
{
    if (globalID.x >= Width || globalID.y >= Height)
        return;

    float2 pix = float2(globalID.xy) + 0.5;
    float2 uv  = pix / float2(Width, Height);
    uv = float2(FromSubUVsToUnit(uv.x, float(Width)),
                FromSubUVsToUnit(uv.y, float(Height)));

    Atmosphere atmosphere = MakeAtmosphere();

    float  cos_sun_zenith = uv.x * 2.0 - 1.0;
    float3 sun_dir        = normalize(float3(0.0,
                                sqrt(saturate(1.0 - cos_sun_zenith * cos_sun_zenith)),
                                cos_sun_zenith));
    float  view_height    = atmosphere.bottom_radius +
                            saturate(uv.y + planetRadiusOffset) *
                            (atmosphere.top_radius - atmosphere.bottom_radius - planetRadiusOffset);

    float3 world_pos = float3(0.0, 0.0, view_height);
    float3 world_dir = ComputeSampleDirection(globalID.z);

    IntegrationResults r = IntegrateScatteredLuminance(world_pos, world_dir, sun_dir, atmosphere);

    shared_multi_scattering[globalID.z] = r.multi_scattering / DIRECTION_SAMPLE_COUNT;
    shared_luminance[globalID.z]        = r.luminance        / DIRECTION_SAMPLE_COUNT;

    GroupMemoryBarrierWithGroupSync();

    // Parallel reduction — sum all 64 direction samples into element 0
    for (uint i = 32u; i > 0u; i >>= 1u)
    {
        if (globalID.z < i)
        {
            shared_multi_scattering[globalID.z] += shared_multi_scattering[globalID.z + i];
            shared_luminance[globalID.z]        += shared_luminance[globalID.z + i];
        }
        GroupMemoryBarrierWithGroupSync();
    }

    if (globalID.z > 0u)
        return;

    float3 luminance = shared_luminance[0] * (1.0 / (1.0 - shared_multi_scattering[0]));
    LUT[globalID.xy] = float4(atmosphere.multiple_scattering_factor * luminance, 1.0);
}
