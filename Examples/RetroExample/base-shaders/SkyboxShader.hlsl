#include "shared.hlsl"

struct PSInput
{
    float4 position : SV_Position;
    float3 world_pos : POSITION0;
    float2 uv : TEXCOORD0;
};

static const float PI = 3.14159265;

PSInput vs_main(Vertex input)
{
    PSInput output;

    float4 dir = float4(input.position, 0.0);

    output.position = mul(dir, camera_buffer.view_proj);
    output.position.z = output.position.w;

    output.world_pos = input.position;
    // U = angle around XZ axis 
    output.uv.x = (atan2(input.position.z, input.position.x) + PI) / (2.0 * PI);
    // V = remapped height
    output.uv.y = input.position.y * 0.5 + 0.5;

    return output;
}

struct PSOutput
{
    float4 color : SV_Target0;
};

PSOutput ps_main(PSInput input)
{
    PSOutput output;

    float2 uv = input.uv;
    float2 p = uv * 40.0;
    float pattern = sin(p.x) * 2.0 + sin(p.y);
    float hatch = step(0.0, pattern);
    // TODO: Multiply by texture color
    output.color = float4(uv, 0.0, 1.0);

    return output;
}
