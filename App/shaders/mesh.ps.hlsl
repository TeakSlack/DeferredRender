#include "common.hlsli"

VK_BINDING(1, 0) cbuffer MaterialCB : register(b1)
{
    float4 g_BaseColorFactor;
};

VK_BINDING(2, 0) Texture2D    g_Albedo  : register(t0);
VK_BINDING(3, 0) SamplerState g_Sampler : register(s0);

struct PSIn
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;
};

float4 main(PSIn input) : SV_Target
{
    return g_Albedo.Sample(g_Sampler, input.texCoord) * g_BaseColorFactor;
}
