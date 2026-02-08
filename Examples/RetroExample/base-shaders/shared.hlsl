#ifndef SHARED_HLSL
#define SHARED_HLSL

#include "shared_structs.h"

cbuffer FrameConstants : register(b0)
{
    CameraMatrices camera_buffer;
    float4x4 cube_world;
    float time;
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