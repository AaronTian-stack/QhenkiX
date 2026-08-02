#include "pbr.slang"
#include "shared.slang"

struct VertexIn
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct PSInput
{
    float4 position : SV_Position;
    float3 world_pos : POSITION0;
    float3 world_pos_no_y : POSITION1;
    float3 normal : NORMAL0;
    float3 instance_color : COLOR0;
    float is_mostly_metal : TEXCOORD0;
};

float hash(float3 p)
{
    return frac(sin(dot(p, float3(127.1, 311.7, 74.7))) * 43758.5453);
}

static const float grid_spacing = 3.0;
static const float grid_half = (grid_size - 1) * grid_spacing * 0.5;
static const float vertical_scale = -3.2;
static const float influence_radius = 10.0;

static const float3 palette[] = {
    float3(76, 62, 36),    // Brown
    float3(251, 185, 84),  // Yellow
    float3(144, 94, 169),  // Purple
    float3(131, 28, 93),   // Dark Purple
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

[shader("vertex")]
PSInput vs_main(VertexIn input, uint instance_id : SV_InstanceID)
{
    PSInput output;
    uint row = instance_id / grid_size;
    uint col = instance_id % grid_size;
    float3 instance_offset = float3(col * grid_spacing - grid_half, 0.0, row * grid_spacing - grid_half);
    float3 cube_position = frame_constants.cube_world[3].xyz;
    float dist = length(instance_offset - cube_position);
    float influence = 1.0 - smoothstep(0.0, influence_radius, dist);
    float vertical_offset = vertical_scale * influence;
    float3 no_y = input.position + instance_offset;
    float3 world_pos = no_y + float3(0.0, vertical_offset, 0.0);
    output.position = mul(float4(world_pos, 1.0), frame_constants.camera_buffer.view_proj);
    output.world_pos = world_pos;
    output.world_pos_no_y = no_y;
    output.normal = normalize(input.normal);
    output.instance_color = palette[hash_to_index(row, col)] / 255.0;
    output.is_mostly_metal = (hash(float3(row, col, 0.0)) > 0.5) ? 1.0 : 0.0;
    return output;
}

static const float light_radius_sq = 32.0;
static const float3 ambient_color = float3(0.25, 0.28, 0.35);
static const float ambient_strength = 0.6;

[shader("fragment")]
float4 ps_main(PSInput input)
    : SV_Target0
{
    float3 V = normalize(frame_constants.camera_position - input.world_pos);
    float3 N = normalize(input.normal);

    // Point light
    float3 light_pos = frame_constants.cube_world[3].xyz;
    float3 L = light_pos - input.world_pos;
    float dist_sq = dot(L, L) + 1e-6;
    float dist = sqrt(dist_sq);
    L /= dist;
    float NdotL = max(0.0, dot(N, L));
    float attenuation = light_intensity / (1.0 + dist_sq / light_radius_sq);
    float illuminance = NdotL * attenuation;

    float roughness = lerp(0.9, 0.2, input.is_mostly_metal);
    float metallic = lerp(0.1, 0.9, input.is_mostly_metal);
    float ao = 0.2;

    Material material;
    material.albedo = input.instance_color;
    material.metallic = metallic;
    material.roughness = roughness;
    material.ao = ao;
    material.f0 = lerp(float3(0.04, 0.04, 0.04), material.albedo, metallic);

    float3 pbr_color = BRDF_CALCULATE(material, illuminance, light_color.rgb, N, L, V);

    float3 ambient = ambient_color * ambient_strength * material.albedo * material.ao;
    return float4(pbr_color + ambient, 1.0);
}
