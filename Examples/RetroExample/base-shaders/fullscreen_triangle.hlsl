struct BlitVSOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

BlitVSOutput vs_main(uint vertex_id : SV_VertexID)
{
    float2 uv = float2((vertex_id << 1) & 2, vertex_id & 2);
    BlitVSOutput output;
    output.position = float4(uv * float2(2, -2) + float2(-1, 1), 0, 1);
    output.uv = uv;
    return output;
}
