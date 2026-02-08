#include "shared.hlsl"

struct PSInput
{
    float4 position : SV_Position;
    float3 normal : COLOR0;
    float2 uv : TEXCOORD0;
};

PSInput vs_main(Vertex input)
{
    PSInput output;

    float4 world_position = float4(input.position, 1.0);
    output.position = mul(world_position, frame_constants.camera_buffer.view_proj);
    output.normal = input.normal;
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

    float3 color = g_texture.Sample(samp, input.uv).rgb;
    output.color = float4(color, 1.0);

    return output;
}
