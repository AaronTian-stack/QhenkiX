#include "shared.slang"

struct PSInput
{
    float4 position : SV_Position;
    float3 world_pos : POSITION0;
};

static const float PI = 3.14159265;
static const float scale_factor = 2.0;

[shader("vertex")]
PSInput vs_main(float3 position : POSITION)
{
    PSInput output;

    float4 dir = float4(position, 0.0);
    dir.y *= scale_factor;

    output.position = mul(dir, frame_constants.camera_buffer.view_proj);
    output.position.z = output.position.w;

    output.world_pos = position;

    return output;
}

#define STEPPED

[shader("fragment")]
float4 ps_main(PSInput input)
    : SV_Target0
{
    float3 dir = normalize(input.world_pos);
    float2 uv;
    uv.x = 1.0 - (atan2(dir.z, dir.x) + PI) / (2.0 * PI);
    uv.y = 0.5 - 0.5 * dir.y;

    const float dot_tiling = 256.0;

    const float speed = 0.02;
    const float step_size = 1 / dot_tiling;
    const float2 raw_offset = float2(frame_constants.time * speed, -frame_constants.time * speed);
    const float2 offset = step_size * floor(raw_offset / step_size);

    float2 uv_d =
#ifdef STEPPED
        uv + offset;
#else
        uv + raw_offset;
#endif

    const float tiling = 4.0;
    float2 p = float2(uv_d.x * tiling, uv_d.y * tiling * 0.5 * scale_factor);
    float2 dot_p = float2(uv.x * dot_tiling, uv.y * dot_tiling * 0.5 * scale_factor);

    float2 cell = frac(dot_p);
    float dist = distance(cell, float2(0.5, 0.5));
    float dot_radius = 0.3;
    float dotted = 1.0 - step(dot_radius, dist);

    float3 tex = g_texture.Sample(samp, p).rgb;

    float dist_from_mid = 2.0 * abs(uv.y - 0.5);
    float alpha_fade = 1.0 - smoothstep(0.5, 1.0, dist_from_mid);

    return float4(dotted * tex.rgb, alpha_fade);
}
