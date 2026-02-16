#ifndef SHARED_HLSL
#define SHARED_HLSL

#include "shared_structs.h"

cbuffer FrameConstantBuffer : register(b0)
{
    FrameConstants frame_constants;
};

Texture2D g_texture : register(t1
#ifdef DX12
                               ,
                               space1);
#else
                      );
#endif

Texture2D g_texture_blur : register(t2
#ifdef DX12
                                    ,
                                    space1);
#else
                           );
#endif

SamplerState samp : register(s0
#ifdef DX12
                             ,
                             space2);
#else
                    );
#endif

SamplerState samp_linear : register(s1
#ifdef DX12
                                    ,
                                    space2);
#else
                           );
#endif

static const float light_intensity = 2.0;
static const float4 light_color = float4(1.0, 0.92, 0.34, 1.0) * light_intensity;

#endif // SHARED_HLSL