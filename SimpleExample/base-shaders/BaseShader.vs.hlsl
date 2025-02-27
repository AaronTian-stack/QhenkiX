//cbuffer CameraBuffer : register(b0)
//{
//    float4x4 view;
//    float4x4 projection;
//};

struct VSInput
{
    float3 position : POSITION;
    float3 color : COLOR0;
};

struct VSOutput
{
    float4 position : SV_Position;
    float3 color : COLOR0;
};

[RootSignature("RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)")]
VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;

    output.position = float4(input.position, 1.0);
    output.color = input.color;

    return output;
}