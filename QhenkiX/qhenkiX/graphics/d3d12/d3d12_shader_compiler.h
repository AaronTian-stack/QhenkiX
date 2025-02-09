#pragma once
#include <wrl/client.h>
#include <dxcapi.h>

#include "graphics/qhenki/shader_compiler.h"

using Microsoft::WRL::ComPtr;

class D3D12ShaderCompiler : public ShaderCompiler
{
	ComPtr<IDxcUtils> m_library_;
	ComPtr<IDxcCompiler3> m_compiler_;

public:
	D3D12ShaderCompiler();
	bool compile(const CompilerInput& input, CompilerOutput& output) override;
	~D3D12ShaderCompiler() override;
};
