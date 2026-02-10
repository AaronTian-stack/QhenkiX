#include "shared.hlsl"

static const float luminance_threshold = 0.25;

struct BlitPSInput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

float4 ps_main(BlitPSInput input)
    : SV_Target0
{
    float4 c = g_texture.Sample(samp_linear, input.uv);
    float L = dot(c.rgb, float3(0.2126, 0.7152, 0.0722));
    if (L < luminance_threshold)
    {
        return float4(0.0, 0.0, 0.0, 0.0);
    }
    return c;
}
