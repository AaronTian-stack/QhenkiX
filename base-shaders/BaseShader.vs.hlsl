struct VSInput
{
    //float3 position : POSITION;
    //float3 color : COLOR0;
	uint vertex_id : SV_VertexID;
};

struct VSOutput
{
    float4 position : SV_Position;
    float3 color : COLOR0;
};

static const float3 positions[3] = {
    float3(0.0, 0.5, 0.0),
    float3(0.5, -0.5, 0.0),
    float3(-0.5, -0.5, 0.0)
};

VSOutput main(VSInput input)
{
    VSOutput output = (VSOutput) 0;
    //output.position = float4(input.position, 1.0);

    // read position from hardcoded array in shader
    output.position = float4(positions[input.vertex_id], 1.0);

    if (input.vertex_id == 0)
    {
        output.color = float3(1.0, 0.0, 0.0); // Solid red
    }
    else if (input.vertex_id == 1)
    {
        output.color = float3(0.0, 1.0, 0.0); // Solid green
    }
    else if (input.vertex_id == 2)
    {
        output.color = float3(0.0, 0.0, 1.0); // Solid blue
    }

    return output;
}