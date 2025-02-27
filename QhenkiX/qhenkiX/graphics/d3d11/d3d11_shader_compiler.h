#pragma once
#include <d3dcommon.h>
#include <wrl/client.h>
#include "graphics/qhenki/shader_compiler.h"

using Microsoft::WRL::ComPtr;

struct D3D11ShaderOutput
{
	ComPtr<ID3DBlob> shader_blob;
	ComPtr<ID3DBlob> root_signature_blob;

	ComPtr<ID3DBlob> debug_info_path;
	ComPtr<ID3DBlob> debug_info_blob;
};

class D3D11ShaderCompiler : public ShaderCompiler
{
public:
	bool compile(const CompilerInput& input, CompilerOutput& output) override;
};
