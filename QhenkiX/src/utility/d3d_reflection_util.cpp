#include "src/utility/d3d_reflection_util.h"

#include <cassert>

namespace qhenki::gfx
{
DXGI_FORMAT mask_to_format(const uint32_t mask, const D3D_REGISTER_COMPONENT_TYPE type)
{
    switch (type)
    {
    case D3D_REGISTER_COMPONENT_UNKNOWN:
        assert(false);
        break;
    case D3D_REGISTER_COMPONENT_UINT32:
        switch (mask)
        {
        case 0x1:
            return DXGI_FORMAT_R32_UINT;
        case 0x3:
            return DXGI_FORMAT_R32G32_UINT;
        case 0x7:
            return DXGI_FORMAT_R32G32B32_UINT;
        case 0xF:
            return DXGI_FORMAT_R32G32B32A32_UINT;
        default:
            assert(false);
            break;
        }
        break;
    case D3D_REGISTER_COMPONENT_SINT32:
        switch (mask)
        {
        case 0x1:
            return DXGI_FORMAT_R32_SINT;
        case 0x3:
            return DXGI_FORMAT_R32G32_SINT;
        case 0x7:
            return DXGI_FORMAT_R32G32B32_SINT;
        case 0xF:
            return DXGI_FORMAT_R32G32B32A32_SINT;
        default:
            assert(false);
            break;
        }
        break;
    case D3D_REGISTER_COMPONENT_FLOAT32:
        switch (mask)
        {
        case 0x1:
            return DXGI_FORMAT_R32_FLOAT;
        case 0x3:
            return DXGI_FORMAT_R32G32_FLOAT;
        case 0x7:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case 0xF:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        default:
            assert(false);
            break;
        }
        break;
    case D3D_REGISTER_COMPONENT_UINT16:
        switch (mask)
        {
        case 0x1:
            return DXGI_FORMAT_R16_UINT;
        case 0x3:
            return DXGI_FORMAT_R16G16_UINT;
        case 0x7:
            assert(false);
            break;
        case 0xF:
            return DXGI_FORMAT_R16G16B16A16_UINT;
        default:
            assert(false);
            break;
        }
        break;
    case D3D_REGISTER_COMPONENT_SINT16:
        switch (mask)
        {
        case 0x1:
            assert(false);
            break;
        case 0x3:
            return DXGI_FORMAT_R16G16_SINT;
        case 0x7:
            assert(false);
            break;
        case 0xF:
            return DXGI_FORMAT_R16G16B16A16_SINT;
        }
        break;
    case D3D_REGISTER_COMPONENT_FLOAT16:
        switch (mask)
        {
        case 0x1:
            return DXGI_FORMAT_R16_FLOAT;
        case 0x3:
            return DXGI_FORMAT_R16G16_FLOAT;
        case 0x7:
            assert(false);
            break;
        case 0xF:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        }
        break;
    case D3D_REGISTER_COMPONENT_UINT64:
    case D3D_REGISTER_COMPONENT_SINT64:
    case D3D_REGISTER_COMPONENT_FLOAT64:
        assert(false);
        break;
    default:
        assert(false);
        break;
    }
    return DXGI_FORMAT_UNKNOWN;
}
} // namespace qhenki::gfx
