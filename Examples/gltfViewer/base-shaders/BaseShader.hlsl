cbuffer CameraBuffer : register(b0)  
{  
    float4x4 viewProj;  
    float4x4 invViewProj;  
    float4x4 proj;
};  

Texture2D g_texture : register(t1);

#ifdef DX12
SamplerState samp : register(s0, space1);
#endif
#ifdef DX11
SamplerState samp : register(s0);
#endif

struct VSInput  
{  
    float3 position : POSITION;
    float3 color : COLOR0;
    float2 uv : TEXCOORD0;
}; 

struct PSInput  
{  
    float4 position : SV_Position;
    float3 color : COLOR0;
    float2 uv : TEXCOORD0;
};
 
PSInput vs_main(VSInput input)
{  
    PSInput output;

    float4 worldPosition = float4(input.position, 1.0);
    output.position = mul(worldPosition, viewProj);
    output.color = input.color;
    output.uv = input.uv;
    
    return output;  
}

struct PSOutput  
{  
    float4 color : SV_Target0;
};

PSOutput ps_main(PSInput input)
{
    PSOutput output;
    
    float3 multColor = g_texture.Sample(samp, input.uv).rgb * input.color;
    output.color = float4(multColor, 1.0);
   
    return output;
}