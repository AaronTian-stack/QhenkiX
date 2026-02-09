#ifndef SHARED_HLSL
#define SHARED_HLSL

#include "shared_structs.h"

cbuffer FrameConstantBuffer : register(b0)
{
    FrameConstants frame_constants;
};

#ifdef DX12
Texture2D g_texture : register(t1, space1);
Texture2D g_texture_blur : register(t2, space1);
SamplerState samp : register(s0, space2);         // nearest (skybox)
SamplerState samp_linear : register(s1, space2);   // linear (blit, cube, etc.)
#else
Texture2D g_texture : register(t1);
Texture2D g_texture_blur : register(t2);
SamplerState samp : register(s0);
SamplerState samp_linear : register(s1);
#endif

static const float light_intensity = 2.0;
static const float4 light_color = float4(1.0, 0.92, 0.34, 1.0) * light_intensity;

#endif // SHARED_HLSL