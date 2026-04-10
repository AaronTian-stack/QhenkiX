#include <qhenki/utility/gfx_util.h>

#include <cassert>

namespace qhenki::gfx
{
#define FORMAT_MAP(X)                                       \
    X(UNKNOWN, DXGI_FORMAT_UNKNOWN)                         \
    X(R32G32B32A32_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT)   \
    X(R32G32B32A32_UINT, DXGI_FORMAT_R32G32B32A32_UINT)     \
    X(R32G32B32A32_SINT, DXGI_FORMAT_R32G32B32A32_SINT)     \
    X(R32G32B32_FLOAT, DXGI_FORMAT_R32G32B32_FLOAT)         \
    X(R32G32B32_UINT, DXGI_FORMAT_R32G32B32_UINT)           \
    X(R32G32B32_SINT, DXGI_FORMAT_R32G32B32_SINT)           \
    X(R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT)   \
    X(R16G16B16A16_UNORM, DXGI_FORMAT_R16G16B16A16_UNORM)   \
    X(R16G16B16A16_UINT, DXGI_FORMAT_R16G16B16A16_UINT)     \
    X(R16G16B16A16_SNORM, DXGI_FORMAT_R16G16B16A16_SNORM)   \
    X(R16G16B16A16_SINT, DXGI_FORMAT_R16G16B16A16_SINT)     \
    X(R32G32_FLOAT, DXGI_FORMAT_R32G32_FLOAT)               \
    X(R32G32_UINT, DXGI_FORMAT_R32G32_UINT)                 \
    X(R32G32_SINT, DXGI_FORMAT_R32G32_SINT)                 \
    X(R10G10B10A2_UNORM, DXGI_FORMAT_R10G10B10A2_UNORM)     \
    X(R10G10B10A2_UINT, DXGI_FORMAT_R10G10B10A2_UINT)       \
    X(R11G11B10_FLOAT, DXGI_FORMAT_R11G11B10_FLOAT)         \
    X(R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM)           \
    X(R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB) \
    X(R8G8B8A8_UINT, DXGI_FORMAT_R8G8B8A8_UINT)             \
    X(R8G8B8A8_SNORM, DXGI_FORMAT_R8G8B8A8_SNORM)           \
    X(R8G8B8A8_SINT, DXGI_FORMAT_R8G8B8A8_SINT)             \
    X(R16G16_FLOAT, DXGI_FORMAT_R16G16_FLOAT)               \
    X(R16G16_UNORM, DXGI_FORMAT_R16G16_UNORM)               \
    X(R16G16_UINT, DXGI_FORMAT_R16G16_UINT)                 \
    X(R16G16_SNORM, DXGI_FORMAT_R16G16_SNORM)               \
    X(R16G16_SINT, DXGI_FORMAT_R16G16_SINT)                 \
    X(D32_FLOAT, DXGI_FORMAT_D32_FLOAT)                     \
    X(R32_FLOAT, DXGI_FORMAT_R32_FLOAT)                     \
    X(R32_UINT, DXGI_FORMAT_R32_UINT)                       \
    X(R32_SINT, DXGI_FORMAT_R32_SINT)                       \
    X(D24_UNORM_S8_UINT, DXGI_FORMAT_D24_UNORM_S8_UINT)     \
    X(R8G8_UNORM, DXGI_FORMAT_R8G8_UNORM)                   \
    X(R8G8_UINT, DXGI_FORMAT_R8G8_UINT)                     \
    X(R8G8_SNORM, DXGI_FORMAT_R8G8_SNORM)                   \
    X(R8G8_SINT, DXGI_FORMAT_R8G8_SINT)                     \
    X(R16_FLOAT, DXGI_FORMAT_R16_FLOAT)                     \
    X(D16_UNORM, DXGI_FORMAT_D16_UNORM)                     \
    X(R16_UNORM, DXGI_FORMAT_R16_UNORM)                     \
    X(R16_UINT, DXGI_FORMAT_R16_UINT)                       \
    X(R16_SNORM, DXGI_FORMAT_R16_SNORM)                     \
    X(R16_SINT, DXGI_FORMAT_R16_SINT)                       \
    X(R8_UNORM, DXGI_FORMAT_R8_UNORM)                       \
    X(BC1_UNORM_SRGB, DXGI_FORMAT_BC1_UNORM_SRGB)           \
    X(BC2_UNORM, DXGI_FORMAT_BC2_UNORM)                     \
    X(BC2_UNORM_SRGB, DXGI_FORMAT_BC2_UNORM_SRGB)           \
    X(BC3_UNORM, DXGI_FORMAT_BC3_UNORM)                     \
    X(BC3_UNORM_SRGB, DXGI_FORMAT_BC3_UNORM_SRGB)           \
    X(BC4_UNORM, DXGI_FORMAT_BC4_UNORM)                     \
    X(BC4_SNORM, DXGI_FORMAT_BC4_SNORM)                     \
    X(BC5_UNORM, DXGI_FORMAT_BC5_UNORM)                     \
    X(BC5_SNORM, DXGI_FORMAT_BC5_SNORM)                     \
    X(BC6H_UF16, DXGI_FORMAT_BC6H_UF16)                     \
    X(BC6H_SF16, DXGI_FORMAT_BC6H_SF16)                     \
    X(BC7_UNORM, DXGI_FORMAT_BC7_UNORM)                     \
    X(BC7_UNORM_SRGB, DXGI_FORMAT_BC7_UNORM_SRGB)

DXGI_FORMAT dxgi_format(const Format format)
{
#define MAP_FORMAT(our, dxgi) \
    case Format::our:         \
        return dxgi;

    switch (format)
    {
        FORMAT_MAP(MAP_FORMAT)
    }
    assert(false); // This should be exhaustive
    return DXGI_FORMAT_UNKNOWN;

#undef MAP_FORMAT
}

Format format_from_dxgi(const DXGI_FORMAT format)
{
#define MAP_DXGI_TO_FORMAT(our, dxgi) \
    case dxgi:                        \
        return Format::our;

    switch (format)
    {
        FORMAT_MAP(MAP_DXGI_TO_FORMAT)
    }
    return Format::UNKNOWN;

#undef MAP_DXGI_TO_FORMAT
}

#undef FORMAT_MAP

} // namespace qhenki::gfx
