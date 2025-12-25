#ifndef SIMPLEEXAMPLE_SHARED_STRUCTS_H
#define SIMPLEEXAMPLE_SHARED_STRUCTS_H

#include <qhenki/utility/hlsl_compat.h>

#ifdef __cplusplus
#include <DirectXMath.h>
using namespace DirectX;
#endif

struct CameraBuffer
{
    XMFLOAT4X4 view_proj;
    XMFLOAT4X4 inv_view_proj;
};

struct VertexInput
{
    XMFLOAT3 position;
    XMFLOAT3 color;
    XMFLOAT2 texcoord;
};

#endif // SIMPLEEXAMPLE_SHARED_STRUCTS_H
