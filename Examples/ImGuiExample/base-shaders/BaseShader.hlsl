struct VSInput  
{  
    float3 position : POSITION;
    float3 color : COLOR0;
}; 

struct PSInput  
{  
    float4 position : SV_Position;
    float3 color : COLOR0;
};
 
PSInput vs_main(VSInput input)
{  
    PSInput output;

    output.position = float4(input.position, 1.0);
    output.color = input.color;
    
    return output;  
}

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