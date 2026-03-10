#ifndef SHARED_HLSL
#define SHARED_HLSL

#include "shared_structs.h"

#ifdef __spirv__
[[vk::binding(0)]]
#endif
cbuffer FrameConstantBuffer : register(b0)
{
    FrameConstants frame_constants;
};

#ifdef __spirv__
[[vk::binding(1, 1)]]
#endif
Texture2D g_texture :
#ifdef DX12
register(t1, space1);
#else
register(t1);
#endif

#ifdef __spirv__
[[vk::binding(2, 1)]]
#endif
Texture2D g_texture_blur :
#ifdef DX12
register(t2, space1);
#else
register(t2);
#endif

#ifdef __spirv__
[[vk::binding(0, 2)]]
#endif
SamplerState samp :
#ifdef DX12
register(s0, space2);
#else
register(s0);
#endif

#ifdef __spirv__
[[vk::binding(1, 2)]]
#endif
SamplerState samp_linear :
#ifdef DX12
register(s1, space2);
#else
register(s1);
#endif

static const float light_intensity = 2.0;
static const float4 light_color = float4(1.0, 0.92, 0.34, 1.0) * light_intensity;

#endif // SHARED_HLSL