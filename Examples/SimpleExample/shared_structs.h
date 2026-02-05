#ifndef SIMPLEEXAMPLE_SHARED_STRUCTS_H
#define SIMPLEEXAMPLE_SHARED_STRUCTS_H

#include <qhenki/utility/hlsl_compat.h>

#ifdef __cplusplus
#include <DirectXMath.h>
using namespace DirectX;
#endif

struct CameraMatrices
{
    XMFLOAT4X4 view_proj;
    XMFLOAT4X4 inv_view_proj;
};

struct Vertex
{
    XMFLOAT3 position
#ifndef __cplusplus
        : POSITION
#endif
        ;
    XMFLOAT3 color
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

#endif // SIMPLEEXAMPLE_SHARED_STRUCTS_H
