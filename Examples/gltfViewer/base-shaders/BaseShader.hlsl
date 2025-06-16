cbuffer CameraBuffer : register(b0)  
{  
    float4x4 viewProj;  
    float4x4 invViewProj;  
    float3 position;
};  

Texture2D g_texture : register(t2);

#ifdef VULKAN
[[vk::push_constant]]
#endif
cbuffer ModelBuffer 
    #ifdef DX12
    : register(b0, space5) // TODO: space5 is hardcoded in implementation, need to change
#endif
#ifdef DX11
    : register(b1)
#endif
{
    float4x4 model;
    float4x4 inverseModel;
};

#ifdef DX12
SamplerState samp : register(s0, space1);
#endif
#ifdef DX11
SamplerState samp : register(s0);
#endif

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

PSOutput ps_main(PSInput input)
{
    PSOutput output;
    
    //float3 multColor = g_texture.Sample(samp, input.uv).rgb * input.color;
    //output.color = float4(input.color, 1.0);
    
    float3 N = normalize(input.normal);
    float lambert = max(0.0, dot(N, normalize(input.cameraPosition - input.position)));
    
    output.color = float4(1.0, 0.0, 0.0, 1.0) * lambert;
   
    return output;
}