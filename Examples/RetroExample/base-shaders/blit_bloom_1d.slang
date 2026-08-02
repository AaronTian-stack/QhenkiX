#include "shared.slang"

struct PSInput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

static const float bilinear_offsets[5] = {-3.229, -1.385, 0.0, 1.385, 3.229};
static const float bilinear_weights[5] = {0.0702, 0.3161, 0.2270, 0.3161, 0.0702};

void pixel(float2 loc, float weight, inout float3 sum_color, inout float sum_a)
{
    float4 t = g_texture.Sample(samp_linear, loc);
    sum_color += t.rgb * weight;
    sum_a += t.a * weight;
}

[shader("fragment")]
float4 ps_main(PSInput input)
    : SV_Target0
{
    uint width, height;
    g_texture.GetDimensions(width, height);
#if BLUR_HORIZONTAL
    float resolution = float(width);
    float h_step = 1.0;
    float v_step = 0.0;
#else
    float resolution = float(height);
    float h_step = 0.0;
    float v_step = 1.0;
#endif
    float step_uv = 1.0 / resolution;
    float2 tc = input.uv;
    float3 sum_color = float3(0.0, 0.0, 0.0);
    float sum_a = 0.0;

    [unroll] for (int i = 0; i < 5; i++)
    {
        float offset = bilinear_offsets[i];
        float2 s = float2(tc.x + offset * step_uv * h_step, tc.y + offset * step_uv * v_step);
        pixel(s, bilinear_weights[i], sum_color, sum_a);
    }

    return float4(sum_color, max(0.0, sum_a));
}
