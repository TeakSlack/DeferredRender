Texture2D<float4> LUT           : register(t0);
SamplerState      LinearSampler : register(s0);

struct PSInput
{
    float4 Position : SV_POSITION;
    float2 UV       : TEXCOORD0;
};

float4 main(PSInput input) : SV_Target
{
    return LUT.Sample(LinearSampler, input.UV);
}
