#include "d3d11_shader.h"

#include <codecvt>
#include <stdexcept>
#include <d3dcompiler.h>

#include "d3d11_context.h"
#include "graphics/shared/d3d_helper.h"

using namespace qhenki::gfx;

D3D11Shader::D3D11Shader(ID3D11Device* const device, const ShaderType shader_type, const std::wstring& name, 
	const CompilerOutput& output, bool& result) : m_type(shader_type)
{
	const auto blob = static_cast<ComPtr<ID3DBlob>*>(output.internal_state.get())->Get();
	assert(blob);

	result = true;
	ID3D11DeviceChild* device_resource = nullptr;
	switch (shader_type)
	{
	case VERTEX_SHADER:
	{
		m_shader = D3D11VertexShader();
		if (FAILED(device->CreateVertexShader(
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			nullptr,
			&std::get<D3D11VertexShader>(m_shader).vertex_shader)))
		{
			result = false;
		}
		else
		{
			device_resource = std::get<D3D11VertexShader>(m_shader).vertex_shader.Get();
			// the blob needs to be saved
			auto& tvs = std::get<D3D11VertexShader>(m_shader);
			tvs.vertex_shader_blob = ComPtr<ID3DBlob>(blob);
		}
		break;
	}
	case PIXEL_SHADER:
	{
		m_shader = ComPtr<ID3D11PixelShader>();
		if (FAILED(device->CreatePixelShader(
			blob->GetBufferPointer(),
			blob->GetBufferSize(),
			nullptr,
			std::get<ComPtr<ID3D11PixelShader>>(m_shader).ReleaseAndGetAddressOf())))
		{
			result = false;
		}
		else
		{
			device_resource = std::get<ComPtr<ID3D11PixelShader>>(m_shader).Get();
		}
		break;
	}
	default:
		throw std::runtime_error("D3D11: Shader type not implemented");
	}

#ifdef _DEBUG
	if (device_resource)
	{
		constexpr size_t max_length = 256;
		char debug_name_str[max_length] = {};
		size_t converted_chars = 0;
		wcstombs_s(&converted_chars, debug_name_str, name.c_str(), max_length - 1);
		D3D11Context::set_debug_name(device_resource, debug_name_str);
	}
#endif
}
