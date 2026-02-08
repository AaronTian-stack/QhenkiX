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

#endif // SHARED_HLSL