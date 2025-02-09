#include "d3d11_shader.h"

#include <codecvt>
#include <stdexcept>
#include <d3dcompiler.h>
#include <iostream>

#include "d3d11_context.h"
#include "graphics/shared/d3d_macros.h"

bool D3D11Shader::compile_shader(const std::wstring& file_name, const std::string& entry_point, const std::string& target_version,
                                 ComPtr<ID3DBlob>& shader_blob, const D3D_SHADER_MACRO* macros)
{
	ComPtr<ID3DBlob> errorBlob = nullptr;
	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;

#if defined(_DEBUG)
	flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	// TODO: d3dcompiler_47.dll should be linked with the application
	HRESULT hr = D3DCompileFromFile(
		file_name.c_str(),
		macros,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entry_point.c_str(),
		target_version.c_str(),
		flags,
		0,
		&shader_blob,
		&errorBlob);
	if (FAILED(hr))
	{
		if (errorBlob)
		{
			std::cerr << (char*)errorBlob->GetBufferPointer() << std::endl;
		}
		return false;
	}
	return true;
}

ComPtr<ID3D11VertexShader> D3D11Shader::vertex_shader(ID3D11Device* const device, const std::wstring& file_name, ComPtr<ID3DBlob> &vertex_shader_blob, const D3D_SHADER_MACRO* macros)
{
	if (!compile_shader(file_name, ENTRYPOINT, VS_VERSION_DX11, vertex_shader_blob, macros))
	{
		std::cerr << "D3D11: Failed to compile vertex shader" << std::endl;
		throw std::runtime_error("D3D11: Failed to compile vertex shader");
	}

	ComPtr<ID3D11VertexShader> vertex_shader;
	if (FAILED(device->CreateVertexShader(
		vertex_shader_blob->GetBufferPointer(),
		vertex_shader_blob->GetBufferSize(),
		nullptr,
		&vertex_shader)))
	{
		// TODO: use default vertex shader fallback
		std::cerr << "D3D11: Failed to create vertex shader" << std::endl;
		throw std::runtime_error("Failed to create vertex shader");
	}

#ifdef _DEBUG
#pragma warning(push)
#pragma warning(disable : 4996)
	if (constexpr size_t max_length = 256; file_name.size() < max_length)
	{
		char debug_name_w[max_length] = {};
		std::wcstombs(debug_name_w, file_name.c_str(), max_length - 1);
		D3D11Context::set_debug_name(vertex_shader.Get(), debug_name_w);
	}
	else std::cerr << "D3D11: Vertex shader debug name is too long" << std::endl;
#endif

	return vertex_shader;
}

ComPtr<ID3D11PixelShader> D3D11Shader::pixel_shader(ID3D11Device* const device, const std::wstring& file_name, const D3D_SHADER_MACRO* macros)
{
	ComPtr<ID3DBlob> pixel_shader_blob;
	if (!compile_shader(file_name, ENTRYPOINT, PS_VERSION_DX11, pixel_shader_blob, macros))
	{
		std::cerr << "D3D11: Failed to compile pixel shader" << std::endl;
		throw std::runtime_error("Failed to compile pixel shader");
	}

	ComPtr<ID3D11PixelShader> pixel_shader;
	if (FAILED(device->CreatePixelShader(
		pixel_shader_blob->GetBufferPointer(),
		pixel_shader_blob->GetBufferSize(),
		nullptr,
		&pixel_shader)))
	{
		// TODO: use default pixel shader fallback
		std::cerr << "D3D11: Failed to create pixel shader" << std::endl;
		throw std::runtime_error("Failed to create pixel shader");
	}

#ifdef _DEBUG
	constexpr size_t maxLength = 256;
	if (file_name.size() >= maxLength)
	{
		std::cerr << "D3D11: Pixel shader debug name is too long" << std::endl;
		throw std::runtime_error("D3D11: Pixel shader debug name is too long");
	}
	char debugName[maxLength] = {};
	std::wcstombs(debugName, file_name.c_str(), maxLength - 1);
	D3D11Context::set_debug_name(pixel_shader.Get(), debugName);
#endif

	return pixel_shader;
}

#pragma warning(pop)
