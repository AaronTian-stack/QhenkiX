#include "shared_structs.h"

#ifdef __spirv__
[[vk::binding(0)]]
#endif
cbuffer CameraBuffer : register(b0)
{
    CameraData camera_data;
};

// Per draw attributes
struct ModelConstants
{
    float4x4 model;
    float4x4 inverse_model;
    int material_index;
};

#ifdef __spirv__
[[vk::push_constant]]
ModelConstants model_constants;
#else
cbuffer ModelBuffer
#ifdef DX11
    : register(b1)
#else
    : register(b0, space5)
#endif
{
    ModelConstants model_constants;
};
#endif

#ifndef DX11
#ifdef __spirv__
[[vk::binding(1)]]
#endif
StructuredBuffer<Texture> textures : register(t1);
#endif
#ifdef __spirv__
[[vk::binding(2)]]
#endif
StructuredBuffer<Material> materials : register(t2);
#ifdef DX11
// Make sure these match in the app
Texture2D base_color_tex : register(t3);
Texture2D metallic_roughness_tex : register(t4);
Texture2D normal_tex : register(t5);
Texture2D occlusion_tex : register(t6);
Texture2D emissive_tex : register(t7);
#else
#ifdef __spirv__
[[vk::binding(3)]]
#endif
Texture2D<float4> g_textures[] : register(t3);
#endif

#ifdef DX11
SamplerState base_color_samp : register(s0);
SamplerState metallic_roughness_samp : register(s1);
SamplerState normal_samp : register(s2);
SamplerState occlusion_samp : register(s3);
SamplerState emissive_samp : register(s4);
#else
#ifdef __spirv__
[[vk::binding(0, 1)]]
#endif
SamplerState samps[] : register(s0, space1);
#endif

// Make sure this matches map in gltf_viewerapp.h
struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL0;
    float3 color : COLOR0;
    float2 uv : TEXCOORD0;
};

struct PSInput
{
    float4 sv_position : SV_Position;
    float3 normal : NORMAL0;
    float3 color : COLOR0;
    float2 uv : TEXCOORD0;

    float3 position : POSITION;
    float3 camera_position : TEXCOORD2;
};

PSInput vs_main(VSInput input)
{
    PSInput output;

    float3x3 mat_n = transpose((float3x3) model_constants.inverse_model);

    float4 model_position = mul(float4(input.position, 1.0), model_constants.model);

    float4 world_position = mul(model_position, camera_data.view_proj);

    output.sv_position = world_position;
    output.position = model_position.xyz;
    output.normal = mul(input.normal, mat_n);
    output.color = input.color;
    output.uv = input.uv;

    output.camera_position = camera_data.position; // Camera position from the constant buffer

    return output;
}

struct PSOutput
{
    float4 color : SV_Target0;
};

void set_values(PSInput input,
                out float4 albedo,
                out float3 normal,
                out float4 metallic_roughness,
                out float AO,
                out float3 emissive)
{
    Material material = materials[model_constants.material_index];
    albedo = material.base_color.factor;
    normal = normalize(input.normal);
    metallic_roughness =
        float4(1.0, material.metallic_roughness.roughness_factor, material.metallic_roughness.metallic_factor, 1.0); // roughness G, metal B
    AO = material.occlusion.strength;
    emissive = material.emissive.factor;

    if (material.base_color.index != -1)
    {
#ifdef DX11
        albedo *= base_color_tex.Sample(base_color_samp, input.uv);
#else
        Texture t = textures[material.base_color.index];
        albedo *= g_textures[t.image_index].Sample(samps[t.sampler_index], input.uv);
#endif
    }
    if (material.normal.index != -1)
    {
        // TBN transform
    }
    if (material.metallic_roughness.index != -1)
    {
#ifdef DX11
        metallic_roughness *= metallic_roughness_tex.Sample(metallic_roughness_samp, input.uv);
#else
        Texture t = textures[material.metallic_roughness.index];
        metallic_roughness *= g_textures[t.image_index].Sample(samps[t.sampler_index], input.uv);
#endif
    }
    if (material.occlusion.index != -1)
    {
#ifdef DX11
        AO = occlusion_tex.Sample(occlusion_samp, input.uv).r * material.occlusion.strength;
#else
        Texture t = textures[material.occlusion.index];
        AO = g_textures[t.image_index].Sample(samps[t.sampler_index], input.uv).r * material.occlusion.strength;
#endif
    }
    if (material.emissive.index != -1)
    {
#ifdef DX11
        emissive *= emissive_tex.Sample(emissive_samp, input.uv).rgb;
#else
        Texture t = textures[material.emissive.index];
        emissive *= g_textures[t.image_index].Sample(samps[t.sampler_index], input.uv).rgb;
#endif
    }
};

PSOutput ps_main(PSInput input)
{
    PSOutput output;

    float3 N = normalize(input.normal);
    float lambert = max(0.0, dot(N, normalize(input.camera_position - input.position)));

    float4 albedo;
    float3 normal;
    float4 metallic_roughness;
    float AO;
    float3 emissive;

    set_values(input, albedo, normal, metallic_roughness, AO, emissive);

    output.color = (albedo + float4(emissive, 0.0)) * lambert;

    return output;
}
