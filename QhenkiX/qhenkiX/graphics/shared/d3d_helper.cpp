#include "d3d_helper.h"

#include <stdexcept>

using namespace qhenki::gfx;

const wchar_t* D3DHelper::get_shader_model_wchar(const ShaderType type, const ShaderModel model)
{
	switch (type)
	{
	case VERTEX_SHADER:
		switch (model)
		{
		case ShaderModel::SM_5_0:
			return L"vs_5_0";
		case ShaderModel::SM_5_1:
			return L"vs_5_1";
		case ShaderModel::SM_6_0:
			return L"vs_6_0";
		case ShaderModel::SM_6_1:
			return L"vs_6_1";
		case ShaderModel::SM_6_2:
			return L"vs_6_2";
		case ShaderModel::SM_6_3:
			return L"vs_6_3";
		case ShaderModel::SM_6_4:
			return L"vs_6_4";
		case ShaderModel::SM_6_5:
			return L"vs_6_5";
		case ShaderModel::SM_6_6:
			return L"vs_6_6";
		}
		break;
	case PIXEL_SHADER:
		switch (model)
		{
		case ShaderModel::SM_5_0:
			return L"ps_5_0";
		case ShaderModel::SM_5_1:
			return L"ps_5_1";
		case ShaderModel::SM_6_0:
			return L"ps_6_0";
		case ShaderModel::SM_6_1:
			return L"ps_6_1";
		case ShaderModel::SM_6_2:
			return L"ps_6_2";
		case ShaderModel::SM_6_3:
			return L"ps_6_3";
		case ShaderModel::SM_6_4:
			return L"ps_6_4";
		case ShaderModel::SM_6_5:
			return L"ps_6_5";
		case ShaderModel::SM_6_6:
			return L"ps_6_6";
		}
		break;
	case COMPUTE_SHADER:
		switch (model)
		{
		case ShaderModel::SM_5_0:
			return L"cs_5_0";
		case ShaderModel::SM_5_1:
			return L"cs_5_1";
		case ShaderModel::SM_6_0:
			return L"cs_6_0";
		case ShaderModel::SM_6_1:
			return L"cs_6_1";
		case ShaderModel::SM_6_2:
			return L"cs_6_2";
		case ShaderModel::SM_6_3:
			return L"cs_6_3";
		case ShaderModel::SM_6_4:
			return L"cs_6_4";
		case ShaderModel::SM_6_5:
			return L"cs_6_5";
		case ShaderModel::SM_6_6:
			return L"cs_6_6";
		}
		break;
	default:
		throw std::runtime_error("D3DHelper: not implemented");
	}
	return nullptr;
}

const char* D3DHelper::get_shader_model_char(const ShaderType type,
	const ShaderModel model)
{
	switch (type)
	{
	case VERTEX_SHADER:
		switch (model)
		{
		case ShaderModel::SM_5_0:
			return "vs_5_0";
		case ShaderModel::SM_5_1:
			return "vs_5_1";
		case ShaderModel::SM_6_0:
			return "vs_6_0";
		case ShaderModel::SM_6_1:
			return "vs_6_1";
		case ShaderModel::SM_6_2:
			return "vs_6_2";
		case ShaderModel::SM_6_3:
			return "vs_6_3";
		case ShaderModel::SM_6_4:
			return "vs_6_4";
		case ShaderModel::SM_6_5:
			return "vs_6_5";
		case ShaderModel::SM_6_6:
			return "vs_6_6";
		}
		break;
	case PIXEL_SHADER:
		switch (model)
		{
		case ShaderModel::SM_5_0:
			return "ps_5_0";
		case ShaderModel::SM_5_1:
			return "ps_5_1";
		case ShaderModel::SM_6_0:
			return "ps_6_0";
		case ShaderModel::SM_6_1:
			return "ps_6_1";
		case ShaderModel::SM_6_2:
			return "ps_6_2";
		case ShaderModel::SM_6_3:
			return "ps_6_3";
		case ShaderModel::SM_6_4:
			return "ps_6_4";
		case ShaderModel::SM_6_5:
			return "ps_6_5";
		case ShaderModel::SM_6_6:
			return "ps_6_6";
		}
		break;
	case COMPUTE_SHADER:
		switch (model)
		{
		case ShaderModel::SM_5_1:
			return "cs_5_1";
		case ShaderModel::SM_6_0:
			return "cs_6_0";
		case ShaderModel::SM_6_1:
			return "cs_6_1";
		case ShaderModel::SM_6_2:
			return "cs_6_2";
		case ShaderModel::SM_6_3:
			return "cs_6_3";
		case ShaderModel::SM_6_4:
			return "cs_6_4";
		case ShaderModel::SM_6_5:
			return "cs_6_5";
		case ShaderModel::SM_6_6:
			return "cs_6_6";
		}
		break;
	default:
		throw std::runtime_error("D3DHelper: not implemented");
	}
	return nullptr;
}

DXGI_FORMAT D3DHelper::get_dxgi_format(const IndexType format)
{
	if (format == IndexType::UINT16)
	{
		return DXGI_FORMAT_R16_UINT;
	}
	return DXGI_FORMAT_R32_UINT;
}

D3D12_PRIMITIVE_TOPOLOGY D3DHelper::get_primitive_topology(const PrimitiveTopology topology)
{
	switch (topology)
	{
	case PrimitiveTopology::POINT_LIST:
		return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	case PrimitiveTopology::LINE_LIST:
		return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
	case PrimitiveTopology::LINE_STRIP:
		return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
	case PrimitiveTopology::TRIANGLE_LIST:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case PrimitiveTopology::TRIANGLE_STRIP:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	default:
		throw std::runtime_error("D3DHelper: Invalid primitive topology");
	}
	return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}
