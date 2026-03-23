#include "shared_structs.h"

#ifdef __spirv__
[[vk::binding(0)]]
#endif
cbuffer CameraBuffer : register(b0)
{
    CameraMatrices camera_buffer;
};

#ifdef __spirv__
[[vk::binding(1)]]
#endif
Texture2D g_texture : register(t1);

#ifdef __spirv__
[[vk::binding(0, 1)]]
#endif
SamplerState samp
#ifdef DX12
    : register(s0, space1);
#else
    : register(s0);
#endif

struct PSInput
{
    float4 position : SV_Position;
    float3 color : COLOR0;
    float2 uv : TEXCOORD0;
};

PSInput vs_main(Vertex input)
{
    PSInput output;

    float4 world_position = float4(input.position, 1.0);
    output.position = mul(world_position, camera_buffer.view_proj);
    output.color = input.color;
    output.uv = input.texcoord;

    return output;
}

struct PSOutput
{
    float4 color : SV_Target0;
};

PSOutput ps_main(PSInput input)
{
    PSOutput output;

    float3 color = g_texture.Sample(samp, input.uv).rgb * input.color;
    output.color = float4(color, 1.0);

    return output;
}
