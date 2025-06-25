#ifdef __INTELLISENSE__
// Visual Studio detects this. Makes writing shader easier
#define DX12
#endif

cbuffer CameraBuffer : register(b0)  
{  
    float4x4 viewProj;  
    float4x4 invViewProj;  
    float3 position;
};

cbuffer ModelBuffer : register(b1)
{
    float4x4 model;
    float4x4 inverseModel;
};

#ifdef VULKAN
[[vk::push_constant]]
#endif
#ifndef DX11
cbuffer MaterialIndex : register(b0, space5) // TODO: space5 is hardcoded in implementation, need to change
{
    int material_index;
};
#endif

struct Material
{
		float4 albedo_factor;
        int albedo_index;
        int albedo_txcs; // Texture coordinate set
    // base_color;
    
		float metallic_factor;
		float roughness_factor;
		int mr_index;
        int mr_txcs;
    // metallic_roughness;

		int index;
		int texture_coordinate_set;
		float scale;
	// normal;

        int occlusion_index;
		int occlusion_txcs;
        float occlusion_strength;
	// occlusion;
    
        float3 emissive_factor;
        int emissive_index;
		int emissive_txcs;
	// emissive;
};

#ifdef DX11
cbuffer MaterialBuffer : register(b2)  
{  
    Material material;  
};
// Make sure these match in the app
Texture2D base_color_tex : register(t3);
Texture2D metallic_roughness_tex : register(t4);
Texture2D normal_tex : register(t5);
Texture2D occlusion_tex : register(t6);
Texture2D emissive_tex : register(t7);
#else
Texture2D<float4> g_textures[] : register(t2);
ConstantBuffer<Material> materials[] : register(b0, space2);
#endif

#ifdef DX11
SamplerState base_color_samp : register(s0);
SamplerState metallic_roughness_samp : register(s1);
SamplerState normal_samp : register(s2);
SamplerState occlusion_samp : register(s3);
SamplerState emissive_samp : register(s4);
#else
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
    float3 cameraPosition : TEXCOORD2;
};
 
PSInput vs_main(VSInput input)
{  
    PSInput output;

    float3x3 mat_n = transpose((float3x3) inverseModel);
    
    float4 modelPosition = mul(float4(input.position, 1.0), model);
    
    float4 worldPosition = mul(modelPosition, viewProj);;
    
    output.sv_position = worldPosition;
    output.position = modelPosition.xyz;
    output.normal = mul(input.normal, mat_n);
    output.color = input.color;
    output.uv = input.uv;
    
    output.cameraPosition = position; // Camera position from the constant buffer
    
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
#ifdef DX12
    Material material = materials[material_index];
#endif
    albedo = material.albedo_factor;
    normal = float3(0.0, 0.0, 0.0); // Normal does not have default REMOVE THIS
    metallic_roughness = float4(1.0, 0.0, 0.0, 1.0);
    AO = 0.0;
    emissive = material.emissive_factor;

    if (material.albedo_index != -1)
    {
    #ifdef DX11
        albedo *= base_color_tex.Sample(base_color_samp, input.uv);
    #else
        albedo *= g_textures[material.albedo_index].Sample(samps[0], input.uv);
    #endif
    }
    if (material.emissive_index != -1)
    {
    #ifdef DX11  
        emissive *= emissive_tex.Sample(emissive_samp, input.uv).rgb;
    #else
        emissive *= g_textures[material.emissive_index].Sample(samps[0], input.uv).rgb;
    #endif
    }
    // TODO: normal needs TBN transform
};

PSOutput ps_main(PSInput input)
{
    PSOutput output;
    
    float3 N = normalize(input.normal);
    float lambert = max(0.0, dot(N, normalize(input.cameraPosition - input.position)));
    
    float4 albedo;
    float3 normal;
    float4 metallic_roughness;
    float AO;
    float3 emissive;
    
    set_values(input, albedo, normal, metallic_roughness, AO, emissive);
    
    output.color = (albedo + float4(emissive, 0.0)) * lambert;
   
    return output;
}