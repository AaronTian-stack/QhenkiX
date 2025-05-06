cbuffer CameraBuffer : register(b0)  
{  
   float4x4 viewProj;  
   float4x4 invViewProj;  
};  

//Texture2D texture : register(t0);

//SamplerState samp : register(s0);

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

//[RootSignature("RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT)")]  
VSOutput vs_main(VSInput input)
{  
   VSOutput output;

   float4 worldPosition = float4(input.position, 1.0);
   output.position = mul(viewProj, worldPosition);
   output.color = input.color;
    
   return output;  
}  

// ----  

struct PSInput  
{  
   float4 position : SV_Position;  
   float3 color : COLOR0;  
};  

struct PSOutput  
{  
   float4 color : SV_Target0;  
};  

PSOutput ps_main(PSInput input)  
{  
   PSOutput output;  
   output.color = float4(input.color, 1.0);  
   return output;  
}