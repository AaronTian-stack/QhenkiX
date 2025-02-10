#pragma once

#define NOMINMAX
#include <d3d11.h>
#include <d3dcommon.h>
#include <string>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

// TODO: rework this class
class D3D11Shader
{
	//// Macros must be null terminated
	//static bool compile_shader(const std::wstring& file_name, const std::string& entry_point, const std::string& target_version,
	//                           ComPtr<ID3DBlob>& shader_blob, const D3D_SHADER_MACRO* macros);

public:
	//ComPtr<ID3DBlob> vertex_blob;
	//ComPtr<ID3D11VertexShader> vertex;
	//ComPtr<ID3D11PixelShader> pixel;

	//static ComPtr<ID3D11VertexShader> vertex_shader(ID3D11Device* const device, const std::wstring& file_name, ComPtr<ID3DBlob>& vertex_shader_blob, const D3D_SHADER_MACRO* macros);
	//static ComPtr<ID3D11PixelShader> pixel_shader(ID3D11Device* const device, const std::wstring& file_name, const D3D_SHADER_MACRO* macros);

	friend class D3D12Context;
};

