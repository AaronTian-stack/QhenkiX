#include "shared.slang"

struct VertexIn
{
    float3 position : POSITION;
};

struct PSInput
{
    float4 position : SV_Position;
    float3 local_position : POSITION0;
};

float noise_3D(float3 p)
{
    float3 i = floor(p);
    float3 f = frac(p);
    f = f * f * (3.0 - 2.0 * f);
    float n = i.x + i.y * 57.0 + i.z * 113.0;
    return lerp(lerp(lerp(frac(sin(n) * 43758.5453), frac(sin(n + 1.0) * 43758.5453), f.x),
                     lerp(frac(sin(n + 57.0) * 43758.5453), frac(sin(n + 58.0) * 43758.5453), f.x),
                     f.y),
                lerp(lerp(frac(sin(n + 113.0) * 43758.5453), frac(sin(n + 114.0) * 43758.5453), f.x),
                     lerp(frac(sin(n + 170.0) * 43758.5453), frac(sin(n + 171.0) * 43758.5453), f.x),
                     f.y),
                f.z);
}

[shader("vertex")]
PSInput vs_main(VertexIn input)
{
    PSInput output;
    float3 pos = input.position;
    output.local_position = pos;

    float3 direction = normalize(float3(pos.x, 0.0, pos.z));
    float noise_value = noise_3D(pos * 2.0 + float3(frame_constants.time * 1.2, frame_constants.time * 1.3, 0.0));
    float displacement = noise_value * 0.5;
    pos += direction * displacement;

    float4 world_position = mul(float4(pos, 1.0), frame_constants.stencil_world);
    output.position = mul(world_position, frame_constants.camera_buffer.view_proj);

    return output;
}

struct PSOutput
{
    float4 color : SV_Target0;
};

static const float bayer[16] = {0.0 / 16.0,
                                8.0 / 16.0,
                                2.0 / 16.0,
                                10.0 / 16.0,
                                12.0 / 16.0,
                                4.0 / 16.0,
                                14.0 / 16.0,
                                6.0 / 16.0,
                                3.0 / 16.0,
                                11.0 / 16.0,
                                1.0 / 16.0,
                                9.0 / 16.0,
                                15.0 / 16.0,
                                7.0 / 16.0,
                                13.0 / 16.0,
                                5.0 / 16.0};

float bayer_eval(uint2 screen_pos)
{
    uint x = screen_pos.x % 4;
    uint y = screen_pos.y % 4;
    return bayer[y * 4 + x];
}

[shader("fragment")]
PSOutput ps_main(PSInput input)
{
    PSOutput output;

    float threshold = bayer_eval(uint2(input.position.xy / 4.0));
    float scale = lerp(1.0, 0.95, (1.0 + sin(frame_constants.time * 2.0)) * 0.5);
    float distance = length(float2(input.local_position.x, input.local_position.z)) * scale;
    float alpha = smoothstep(1.0, 0.0, pow(abs(distance), 2.0));

    if (alpha < threshold)
    {
        discard;
    }

    output.color = float4(0.0, 0.0, 0.0, 0.0);
    return output;
}
