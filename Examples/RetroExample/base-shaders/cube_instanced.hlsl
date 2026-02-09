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
    float3 instance_color : COLOR0;
};

float hash(float3 p)
{
    return frac(sin(dot(p, float3(127.1, 311.7, 74.7))) * 43758.5453);
}

// Adapted from https://www.shadertoy.com/view/4dS3Wd
float value_noise_3d(float3 p)
{
    float3 i = floor(p);
    float3 f = frac(p);
    f = f * f * (3.0 - 2.0 * f);

    float n000 = hash(i);
    float n100 = hash(i + float3(1, 0, 0));
    float n010 = hash(i + float3(0, 1, 0));
    float n110 = hash(i + float3(1, 1, 0));
    float n001 = hash(i + float3(0, 0, 1));
    float n101 = hash(i + float3(1, 0, 1));
    float n011 = hash(i + float3(0, 1, 1));
    float n111 = hash(i + float3(1, 1, 1));

    return lerp(
        lerp(lerp(n000, n100, f.x), lerp(n010, n110, f.x), f.y),
        lerp(lerp(n001, n101, f.x), lerp(n011, n111, f.x), f.y),
        f.z);
}

static const float grid_spacing = 3.0;
static const float grid_half = (grid_size - 1) * grid_spacing * 0.5;
static const float vertical_scale = 3.0;
static const float influence_radius = 10.0;

static const float3 palette[] =
{
    float3(76, 62, 36), // Brown
    float3(251, 185, 84), // Yellow
    float3(144, 94, 169), // Purple
    float3(131, 28, 93), // Dark Purple
    float3(155, 171, 178), // Gray
};
static const uint palette_count = 5;

uint hash_to_index(uint row, uint col)
{
    uint n = row * grid_size + col;
    n = (n ^ 61u) ^ (n >> 16u);
    n = n * 9u;
    n = n ^ (n >> 4u);
    n = n * 0x27d4eb2du;
    n = n ^ (n >> 15u);
    return n % palette_count;
}

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
    float3 cube_position = frame_constants.cube_world[3].xyz;
    float dist = length(instance_offset - cube_position);
    float influence = 1.0 - smoothstep(0.0, influence_radius, dist);
    float height_diff = instance_offset.y - cube_position.y;
    float vertical_offset = height_diff * vertical_scale * influence;
    float3 world_pos = input.position + instance_offset + float3(0.0, vertical_offset, 0.0);
    output.position = mul(float4(world_pos, 1.0), frame_constants.camera_buffer.view_proj);
    output.world_pos = world_pos;
    output.normal = normalize(input.normal);
    output.instance_color = palette[hash_to_index(row, col)] / 255.0;
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
    float3 lit = input.instance_color * NdotL;
    output.color = float4(lit, 1.0);
    return output;
}
