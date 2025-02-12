#include "d3d_helper.h"

#include <stdexcept>

using namespace qhenki::graphics;

const wchar_t* D3DHelper::get_shader_model_wchar(const qhenki::graphics::ShaderType type, const qhenki::graphics::ShaderModel model)
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
}

