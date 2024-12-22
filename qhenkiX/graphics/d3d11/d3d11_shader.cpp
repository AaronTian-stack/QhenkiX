#include "d3d11_shader.h"

#include <codecvt>
#include <stdexcept>
#include <d3dcompiler.h>
#include <iostream>

#include "d3d11_context.h"

bool D3D11Shader::compile_shader(const std::wstring& fileName, const std::string& entryPoint, const std::string& profile,
                                 ComPtr<ID3DBlob>& shaderBlob, const D3D_SHADER_MACRO* macros)
{
	ComPtr<ID3DBlob> errorBlob = nullptr;
	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;

#if defined(_DEBUG)
	flags |= D3DCOMPILE_DEBUG;
#endif

	HRESULT hr = D3DCompileFromFile(
		fileName.c_str(),
		macros,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entryPoint.c_str(),
		profile.c_str(),
		flags,
		0,
		&shaderBlob,
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

#pragma warning(push)
#pragma warning(disable : 4996)

ComPtr<ID3D11VertexShader> D3D11Shader::vertex_shader(const ComPtr<ID3D11Device>& device, const std::wstring& file_name, ComPtr<ID3DBlob> &vertex_shader_blob, const D3D_SHADER_MACRO* macros)
{
	if (!compile_shader(file_name, ENTRYPOINT, VS_VERSION, vertex_shader_blob, macros))
	{
		throw std::runtime_error("Failed to compile vertex shader");
	}

	ComPtr<ID3D11VertexShader> vertex_shader;
	if (FAILED(device->CreateVertexShader(
		vertex_shader_blob->GetBufferPointer(),
		vertex_shader_blob->GetBufferSize(),
		nullptr,
		&vertex_shader)))
	{
		throw std::runtime_error("Failed to create vertex shader");
	}

#ifdef _DEBUG
	constexpr size_t maxLength = 256;
	if (file_name.size() >= maxLength)
	{
		throw std::runtime_error("D3D11: Vertex shader debug name is too long");
	}
	char debugName[maxLength] = {};
	std::wcstombs(debugName, file_name.c_str(), maxLength - 1);
	D3D11Context::set_debug_name(vertex_shader.Get(), debugName);
#endif

	return vertex_shader;
}

ComPtr<ID3D11PixelShader> D3D11Shader::pixel_shader(const ComPtr<ID3D11Device>& device, const std::wstring& file_name, const D3D_SHADER_MACRO* macros)
{
	ComPtr<ID3DBlob> pixel_shader_blob;
	if (!compile_shader(file_name, ENTRYPOINT, PS_VERSION, pixel_shader_blob, macros))
	{
		throw std::runtime_error("Failed to compile pixel shader");
	}

	ComPtr<ID3D11PixelShader> pixel_shader;
	if (FAILED(device->CreatePixelShader(
		pixel_shader_blob->GetBufferPointer(),
		pixel_shader_blob->GetBufferSize(),
		nullptr,
		&pixel_shader)))
	{
		throw std::runtime_error("Failed to create pixel shader");
	}

#ifdef _DEBUG
	constexpr size_t maxLength = 256;
	if (file_name.size() >= maxLength)
	{
		throw std::runtime_error("D3D11: Pixel shader debug name is too long");
	}
	char debugName[maxLength] = {};
	std::wcstombs(debugName, file_name.c_str(), maxLength - 1);
	D3D11Context::set_debug_name(pixel_shader.Get(), debugName);
#endif

	return pixel_shader;
}

#pragma warning(pop)
