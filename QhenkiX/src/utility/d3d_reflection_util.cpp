#include "src/utility/d3d_reflection_util.h"

#include <bit>

namespace qhenki::gfx
{
DXGI_FORMAT mask_to_format(const uint32_t mask, const D3D_REGISTER_COMPONENT_TYPE type)
{
    const auto component_count = static_cast<uint32_t>(std::popcount(mask));
    switch (type)
    {
    case D3D_REGISTER_COMPONENT_UINT32:
        switch (component_count)
        {
        case 1:
            return DXGI_FORMAT_R32_UINT;
        case 2:
            return DXGI_FORMAT_R32G32_UINT;
        case 3:
            return DXGI_FORMAT_R32G32B32_UINT;
        case 4:
            return DXGI_FORMAT_R32G32B32A32_UINT;
        }
        break;
    case D3D_REGISTER_COMPONENT_SINT32:
        switch (component_count)
        {
        case 1:
            return DXGI_FORMAT_R32_SINT;
        case 2:
            return DXGI_FORMAT_R32G32_SINT;
        case 3:
            return DXGI_FORMAT_R32G32B32_SINT;
        case 4:
            return DXGI_FORMAT_R32G32B32A32_SINT;
        }
        break;
    case D3D_REGISTER_COMPONENT_FLOAT32:
        switch (component_count)
        {
        case 1:
            return DXGI_FORMAT_R32_FLOAT;
        case 2:
            return DXGI_FORMAT_R32G32_FLOAT;
        case 3:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case 4:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        }
        break;
    case D3D_REGISTER_COMPONENT_UINT16:
        switch (component_count)
        {
        case 1:
            return DXGI_FORMAT_R16_UINT;
        case 2:
            return DXGI_FORMAT_R16G16_UINT;
        case 4:
            return DXGI_FORMAT_R16G16B16A16_UINT;
        }
        break;
    case D3D_REGISTER_COMPONENT_SINT16:
        switch (component_count)
        {
        case 1:
            return DXGI_FORMAT_R16_SINT;
        case 2:
            return DXGI_FORMAT_R16G16_SINT;
        case 4:
            return DXGI_FORMAT_R16G16B16A16_SINT;
        }
        break;
    case D3D_REGISTER_COMPONENT_FLOAT16:
        switch (component_count)
        {
        case 1:
            return DXGI_FORMAT_R16_FLOAT;
        case 2:
            return DXGI_FORMAT_R16G16_FLOAT;
        case 4:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        }
        break;
    default:
        break;
    }
    return DXGI_FORMAT_UNKNOWN;
}
} // namespace qhenki::gfx
