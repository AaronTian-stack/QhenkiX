#ifndef RETROEXAMPLE_SHARED_STRUCTS_H
#define RETROEXAMPLE_SHARED_STRUCTS_H

#include <qhenki/utility/hlsl_compat.h>

#ifdef __cplusplus
#include <qhenki/utility/directxmath_compat.h>
using namespace DirectX;
#endif

struct CameraMatrices
{
    XMFLOAT4X4 view_proj;
    XMFLOAT4X4 inv_view_proj;
};

struct FrameConstants
{
    CameraMatrices camera_buffer;
    XMFLOAT4X4 cube_world;
    XMFLOAT4X4 stencil_world;
    XMFLOAT3 camera_position;
    float time;
};

static
#ifdef __cplusplus
    constexpr
#else
    const
#endif
    int grid_size = 10;

struct Vertex
{
    XMFLOAT3 position
#ifndef __cplusplus
        : POSITION
#endif
        ;
    XMFLOAT3 normal
#ifndef __cplusplus
        : COLOR0
#endif
        ;
    XMFLOAT2 texcoord
#ifndef __cplusplus
        : TEXCOORD0
#endif
        ;
};

#endif // RETROEXAMPLE_SHARED_STRUCTS_H
