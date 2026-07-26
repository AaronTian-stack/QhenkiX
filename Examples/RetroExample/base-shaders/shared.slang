#ifndef SHARED_SLANG
#define SHARED_SLANG

#include "shared_structs.h"

cbuffer FrameConstantBuffer : register(b0)
{
    FrameConstants frame_constants;
};

Texture2D g_texture
#ifdef DX11
    : register(t1);
#else
    : register(t1, space1);
#endif

Texture2D g_texture_blur
#ifdef DX11
    : register(t2);
#else
    : register(t2, space1);
#endif

SamplerState samp
#ifdef DX11
    : register(s0);
#else
    : register(s0, space2);
#endif

SamplerState samp_linear
#ifdef DX11
    : register(s1);
#else
    : register(s1, space2);
#endif

static const float light_intensity = 2.0;
static const float4 light_color = float4(1.0, 0.92, 0.34, 1.0) * light_intensity;

#endif // SHARED_SLANG
