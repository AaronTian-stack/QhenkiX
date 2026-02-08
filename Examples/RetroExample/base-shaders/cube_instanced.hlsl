#include "shared.hlsl"

struct VertexIn
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct PSInput
{
    float4 position : SV_Position;
    float3 world_pos : POSITION0;
    float3 normal : NORMAL0;
};

static const float grid_spacing = 3.0;
static const float grid_half = (grid_size - 1) * grid_spacing * 0.5;

PSInput vs_main(VertexIn input, uint instance_id : SV_InstanceID)
{
    PSInput output;
    uint row = instance_id / grid_size;
    uint col = instance_id % grid_size;
    float3 instance_offset = float3(
        col * grid_spacing - grid_half,
        0.0,
        row * grid_spacing - grid_half
    );
    float3 world_pos = input.position + instance_offset;
    output.position = mul(float4(world_pos, 1.0), frame_constants.camera_buffer.view_proj);
    output.world_pos = world_pos;
    output.normal = normalize(input.normal);
    return output;
}

struct PSOutput
{
    float4 color : SV_Target0;
};

PSOutput ps_main(PSInput input)
{
    PSOutput output;
    float3 view_dir = normalize(frame_constants.camera_position - input.world_pos);
    float3 n = normalize(input.normal);
    float NdotL = max(0.0, dot(n, view_dir));
    output.color = float4(NdotL, NdotL, NdotL, 1.0);
    return output;
}
