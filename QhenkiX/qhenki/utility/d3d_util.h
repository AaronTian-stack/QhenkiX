#pragma once

#include <directx/dxgiformat.h>
#include <wrl/client.h>
#include <string>

#include "qhenki/RHI/barrier.h"
#include "qhenki/RHI/enums.h"
#include "qhenki/RHI/sampler.h"
#include "qhenki/RHI/shader.h"

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
std::wstring get_shader_model_wchar(ShaderType type, ShaderModel model);
std::string get_shader_model_char(ShaderType type, ShaderModel model);
DXGI_FORMAT get_dxgi_format(IndexType format);
D3D12_PRIMITIVE_TOPOLOGY get_primitive_topology(PrimitiveTopology topology);

D3D12_BARRIER_SYNC sync_stage(SyncStage stage);
D3D12_BARRIER_ACCESS access_flags(AccessFlags access);
D3D12_BARRIER_LAYOUT layout(Layout layout);

// Only returns the filter types shared with D3D11
D3D12_FILTER filter(Filter min, Filter mag, Filter mip, ComparisonFunc func, UINT max_anisotropy);
D3D12_TEXTURE_ADDRESS_MODE texture_address_mode(AddressMode mode);
D3D12_COMPARISON_FUNC comparison_func(ComparisonFunc func);

bool is_depth_stencil_format(DXGI_FORMAT format);
} // namespace qhenki::gfx
