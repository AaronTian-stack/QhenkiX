#ifndef GLTFVIEWER_SHARED_STRUCTS_H
#define GLTFVIEWER_SHARED_STRUCTS_H

#include <qhenki/utility/hlsl_compat.h>

#ifdef __cplusplus
#include <DirectXMath.h>
using namespace DirectX;
#endif

struct CameraData
{
    XMFLOAT4X4 view_proj;
    XMFLOAT4X4 inv_view_proj;
    XMFLOAT3 position;
};

#endif // GLTFVIEWER_SHARED_STRUCTS_H
