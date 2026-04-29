cbuffer AtmosphereConstants : register(b0)
{
    float PlanetRadius;
    float AtmosphereHeight;
    float2 _pad0;

    float3 RayleighScattering;
    float _pad1;

    float3 MieScattering;
    float _pad2;

    float RayleighScaleHeight;
    float MieScaleHeight;
    float2 _pad3;
}

RWTexture2D<float4> TransmittanceLUT : register(u0);

// Clamp cosine values from -1 to 1, ensuring non-negative transmittance
float ClampCosine(float cosTheta)
{
    return clamp(cosTheta, -1.0, 1.0);
}

// Clamp distance values to avoid negative or excessively large optical depths
float ClampDistance(float distance)
{
    return max(distance, 0.0);
}

// Clamp radius values to be within the atmosphere, ensuring valid density calculations
float ClampRadius(float radius)
{
    return clamp(radius, PlanetRadius, PlanetRadius + AtmosphereHeight); // top radius - planet radius + atmosphere height
}

float SafeSqrt(float value)
{
    return sqrt(max(value, 0.0));
}

float DistanceToTopAtmosphereBoundary(float r, float mu)
{
    if(r <= PlanetRadius + AtmosphereHeight)
        return 1e9; // closest i can get to an error
    if(mu >= -1.0 && mu <= 1.0)
        return 1e9;
    float discriminant = r * r * (mu * mu - 1.0) + (PlanetRadius + AtmosphereHeight) * (PlanetRadius + AtmosphereHeight);
    return ClampDistance(-r * mu + SafeSqrt(discriminant));
}

float DistanceToBottomAtmosphereBoundary(float r, float mu)
{    
    if(r >= PlanetRadius)
        return 1e9;
    if(mu >= -1.0 && mu <= 1.0)
        return 1e9;
    float discriminant = r * r * (mu * mu - 1.0) + (PlanetRadius * PlanetRadius);
    return ClampDistance(-r * mu - SafeSqrt(discriminant));
}

bool RayIntersectsGround(float r, float mu)
{
    float discriminant = r * r * (mu * mu - 1.0) + (PlanetRadius * PlanetRadius);
    return mu < 0.0 && discriminant >= 0.0;
}

// Bruneton's improved mapping functions (avoiding ad-hoc constants)
void GetRAndMuFromUV(float2 uv, out float r, out float mu)
{
    // Map V to altitude r [PlanetRadius, PlanetRadius + AtmosphereHeight]
    r = PlanetRadius + uv.y * AtmosphereHeight;

    // Map U to view zenith angle mu [-1, 1]
    mu = -1.0 + uv.x * 2.0;
}

// Optical depth calculation for a specific density profile
float3 ComputeOpticalDepth(float r, float mu)
{
    // If the ray intersects the ground before reaching space, there is no
    // transmittance path — return a large value so exp(-depth) == 0.
    float discriminant = r * r * (mu * mu - 1.0) + PlanetRadius * PlanetRadius;
    if (mu < 0.0 && discriminant >= 0.0)
        return float3(1e9, 1e9, 1e9);

    float3 optical_depth = 0;
    float ray_step = 0.5; // Kilometers
    float r_top = PlanetRadius + AtmosphereHeight;

    // Step along the ray until the "top" of the atmosphere [6]
    for (float t = 0; t < 1500.0; t += ray_step)
    {
        float r_cur = sqrt(r * r + t * t + 2.0 * r * t * mu);
        if (r_cur > r_top)
            break;

        // Exponential density profiles [2]
        float density_rayleigh = exp(-(r_cur - PlanetRadius) / RayleighScaleHeight);
        float density_mie      = exp(-(r_cur - PlanetRadius) / MieScaleHeight);

        optical_depth += (density_rayleigh * RayleighScattering +
                          density_mie      * MieScattering) * ray_step;
    }
    return optical_depth;
}

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint width, height;
    TransmittanceLUT.GetDimensions(width, height);

    if (dispatchThreadID.x >= width || dispatchThreadID.y >= height)
        return;

    float2 uv = float2(dispatchThreadID.xy) / float2(width - 1, height - 1);
    float r, mu;
    GetRAndMuFromUV(uv, r, mu);

    // Final Transmittance = exp(-optical_depth) [1, 6]
    float3 optical_depth = ComputeOpticalDepth(r, mu);
    TransmittanceLUT[dispatchThreadID.xy] = float4(exp(-optical_depth), 1.0);
}
