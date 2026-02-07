#ifndef SHARED_HLSL
#define SHARED_HLSL

#include "shared_structs.h"

cbuffer CameraBuffer : register(b0)
{
    CameraMatrices camera_buffer;
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