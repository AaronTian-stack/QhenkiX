#ifndef GLTFVIEWER_SHARED_STRUCTS_H
#define GLTFVIEWER_SHARED_STRUCTS_H

#include <qhenki/utility/hlsl_compat.h>

#ifdef __cplusplus
#include <qhenki/utility/directxmath_compat.h>
using namespace DirectX;
#define F4_1 = (*reinterpret_cast<const XMFLOAT4*>(&g_XMOne))
#define F3_0 = (*reinterpret_cast<const XMFLOAT3*>(&g_XMZero))
#define F_0 = (reinterpret_cast<const XMFLOAT4*>(&g_XMZero)->x)
#define F_1 = (reinterpret_cast<const XMFLOAT4*>(&g_XMOne)->x)
#define NEG_1 = -1
#define ZERO = 0
#else
#define F4_1
#define F3_0
#define F_0
#define F_1
#define NEG_1
#define ZERO
#endif

struct CameraData
{
    XMFLOAT4X4 view_proj;
    XMFLOAT4X4 inv_view_proj;
    XMFLOAT3 position;
};
struct ModelConstants
{
    XMFLOAT4X3 model;
    XMFLOAT4X3 inverse_model;
    int material_index;
};

#ifdef __cplusplus
static_assert(sizeof(ModelConstants) <= 128);
#endif

struct BaseColor
{
    XMFLOAT4 factor F4_1;
    int index NEG_1;
    int texture_coordinate_set ZERO;
};

struct MetallicRoughness
{
    float metallic_factor F_0;
    float roughness_factor F_0;
    int index NEG_1;
    int texture_coordinate_set ZERO;
};

struct Normal
{
    int index NEG_1;
    int texture_coordinate_set ZERO;
    float scale F_1;
};

struct Occlusion
{
    int index NEG_1;
    int texture_coordinate_set ZERO;
    float strength F_1;
};

struct Emissive
{
    XMFLOAT3 factor F3_0;
    int index NEG_1;
    int texture_coordinate_set ZERO;
};

struct Material
{
    BaseColor base_color;
    MetallicRoughness metallic_roughness;
    Normal normal;
    Occlusion occlusion;
    Emissive emissive;
};

#if defined(__cplusplus) || !defined(DX11)
struct Texture
{
    int image_index NEG_1;
    int sampler_index NEG_1;
};
#endif

#undef F4_1
#undef F3_0
#undef F_0
#undef F_1
#undef NEG_1
#undef ZERO

#endif // GLTFVIEWER_SHARED_STRUCTS_H
