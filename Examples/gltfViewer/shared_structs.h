#ifndef GLTFVIEWER_SHARED_STRUCTS_H
#define GLTFVIEWER_SHARED_STRUCTS_H

#include <qhenki/utility/hlsl_compat.h>

#ifdef __cplusplus
#include <qhenki/utility/directxmath_compat.h>
using namespace DirectX;
#define F4_1 = {1.0f, 1.0f, 1.0f, 1.0f}
#define F3_0 = {0.f, 0.f, 0.f}
#define ONE = 1
#define NEG_ONE = -1
#define ZERO = 0
#else
#define F4_1
#define F3_0
#define ONE
#define NEG_ONE
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
    int index NEG_ONE;
    int texture_coordinate_set ZERO;
};

struct MetallicRoughness
{
    float metallic_factor ONE;
    float roughness_factor ONE;
    int index NEG_ONE;
    int texture_coordinate_set ZERO;
};

struct Normal
{
    int index NEG_ONE;
    int texture_coordinate_set ZERO;
    float scale ONE;
};

struct Occlusion
{
    int index NEG_ONE;
    int texture_coordinate_set ZERO;
    float strength ONE;
};

struct Emissive
{
    XMFLOAT3 factor F3_0;
    int index NEG_ONE;
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
    int image_index NEG_ONE;
    int sampler_index NEG_ONE;
};
#endif

#undef F4_1
#undef F3_0
#undef ONE
#undef NEG_ONE
#undef ZERO

#endif // GLTFVIEWER_SHARED_STRUCTS_H
