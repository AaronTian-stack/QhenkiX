#ifndef SHARED_HLSL
#define SHARED_HLSL

#include "shared_structs.h"

cbuffer FrameConstantBuffer : register(b0)
{
    FrameConstants frame_constants;
};

Texture2D g_texture : register(t1);

SamplerState samp : register(s0
#ifdef DX12
, space1);
#endif
#ifdef DX11
);
#endif

static const float3 light_color = float3(1.0, 1.0, 0.9);

#endif // SHARED_HLSL