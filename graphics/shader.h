#pragma once

#include <d3d11.h>
#include <d3dcommon.h>
#include <string>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

#define VS_VERSION "vs_5_0"
#define PS_VERSION "ps_5_0"
#define ENTRYPOINT "main"

class Shader
{
	// macros must be null terminated
	static bool compile_shader(const std::wstring& fileName, const std::string& entryPoint, const std::string& profile,
		ComPtr<ID3DBlob>& shaderBlob, const D3D_SHADER_MACRO* macros);

public:
	static ComPtr<ID3D11VertexShader> vertex_shader(const ComPtr<ID3D11Device>& device, const std::wstring& fileName, ComPtr<ID3DBlob>& vertexShaderBlob, const D3D_SHADER_MACRO* macros);
	static ComPtr<ID3D11PixelShader> pixel_shader(const ComPtr<ID3D11Device>& device, const std::wstring& fileName, const D3D_SHADER_MACRO* macros);
};

