#pragma once
#include <wrl/client.h>
#include <dxcapi.h>
#include <dxgiformat.h>

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
	ComPtr<IDxcUtils> m_library_;
	ComPtr<IDxcCompiler3> m_compiler_;

	static DXGI_FORMAT mask_to_format(const uint32_t mask);

public:
	D3D12ShaderCompiler();
	bool compile(const CompilerInput& input, CompilerOutput& output) override;
	~D3D12ShaderCompiler() override;

	friend class D3D12Context;
};
