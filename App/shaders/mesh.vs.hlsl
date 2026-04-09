#pragma pack_matrix(column_major)
#include "common.hlsli"

VK_BINDING(0, 0) cbuffer TransformCB : register(b0)
{
    float4x4 g_MVP;
};

struct VSIn
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texCoord : TEXCOORD;
    float3 tangent  : TANGENT;
};

struct VSOut
{
    float4 position : SV_Position;
    float2 texCoord : TEXCOORD;
};

VSOut main(VSIn input)
{
    VSOut output;
    output.position = mul(g_MVP, float4(input.position, 1.0f));
    output.texCoord = input.texCoord;
    return output;
}
