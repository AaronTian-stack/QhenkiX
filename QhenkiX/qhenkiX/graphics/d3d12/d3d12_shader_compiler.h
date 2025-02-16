#pragma once
#include <d3dcommon.h>
#include <wrl/client.h>
#include <dxcapi.h>
#include <dxgiformat.h>

#include "graphics/d3d11/d3d11_shader_compiler.h"
#include "graphics/qhenki/shader_compiler.h"

using Microsoft::WRL::ComPtr;

struct D3D12ShaderOutput
{
	ComPtr<IDxcBlob> shader_blob;
	ComPtr<IDxcBlob> reflection_blob;
	ComPtr<IDxcBlob> root_signature_blob;
};

class D3D12ShaderCompiler : public ShaderCompiler
{
	D3D11ShaderCompiler m_d3d11_shader_compiler_;

	ComPtr<IDxcUtils> m_library_;
	ComPtr<IDxcCompiler3> m_compiler_; // Not thread safe

	static DXGI_FORMAT mask_to_format(const uint32_t mask, const D3D_REGISTER_COMPONENT_TYPE type);

public:
	D3D12ShaderCompiler();
	bool compile(const CompilerInput& input, CompilerOutput& output) override;
	~D3D12ShaderCompiler() override;

	friend class D3D12Context;
};
