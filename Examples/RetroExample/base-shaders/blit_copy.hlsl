#include "shared.hlsl"

struct BlitPSInput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

// Adapted from https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
static float3 ACESFilm(float3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

static const float GAMMA = 2.2;

float4 ps_main(BlitPSInput input)
    : SV_Target0
{
    float4 base = g_texture.Sample(samp_linear, input.uv);
    float4 bloom = g_texture_blur.Sample(samp_linear, input.uv);
    float3 combined = base.rgb + bloom.rgb;
    float3 ldr = ACESFilm(combined);
    float3 gamma_corrected = pow(ldr, 1.0 / GAMMA);

    // Adapted from https://www.shadertoy.com/view/lsKSWR
    float2 uv_m = input.uv * (1.0 - input.uv.yx);
    float vig = uv_m.x * uv_m.y * 15.0;
    vig = pow(abs(vig), 0.15);

    return float4(gamma_corrected * vig, base.a);
}
