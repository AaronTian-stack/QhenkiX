#include "d3d11_shader.h"

#include <stdexcept>
#include <d3dcompiler.h>
#include <filesystem>
#include <iostream>

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

ComPtr<ID3D11VertexShader> D3D11Shader::vertex_shader(const ComPtr<ID3D11Device>& device, const std::wstring& fileName, ComPtr<ID3DBlob> &vertexShaderBlob, const D3D_SHADER_MACRO* macros)
{
	if (!compile_shader(fileName, ENTRYPOINT, VS_VERSION, vertexShaderBlob, macros))
	{
		throw std::runtime_error("Failed to compile vertex shader");
	}

	ComPtr<ID3D11VertexShader> vertexShader;
	if (FAILED(device->CreateVertexShader(
		vertexShaderBlob->GetBufferPointer(),
		vertexShaderBlob->GetBufferSize(),
		nullptr,
		&vertexShader)))
	{
		throw std::runtime_error("Failed to create vertex shader");
	}
	return vertexShader;
}

ComPtr<ID3D11PixelShader> D3D11Shader::pixel_shader(const ComPtr<ID3D11Device>& device, const std::wstring& fileName, const D3D_SHADER_MACRO* macros)
{
	ComPtr<ID3DBlob> pixelShaderBlob;
	if (!compile_shader(fileName, ENTRYPOINT, PS_VERSION, pixelShaderBlob, macros))
	{
		throw std::runtime_error("Failed to compile pixel shader");
	}

	ComPtr<ID3D11PixelShader> pixelShader;
	if (FAILED(device->CreatePixelShader(
		pixelShaderBlob->GetBufferPointer(),
		pixelShaderBlob->GetBufferSize(),
		nullptr,
		&pixelShader)))
	{
		throw std::runtime_error("Failed to create pixel shader");
	}
	return pixelShader;
}
