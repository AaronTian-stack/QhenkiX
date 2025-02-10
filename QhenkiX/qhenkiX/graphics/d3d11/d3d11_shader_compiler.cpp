#include "d3d11_shader_compiler.h"

#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <iostream>
#include <wrl/client.h>

#include "graphics/shared/d3d_helper.h"

using Microsoft::WRL::ComPtr;

bool D3D11ShaderCompiler::compile(const CompilerInput& input, CompilerOutput& output)
{
	ComPtr<ID3DBlob> error_blob = nullptr;
	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;

#if defined(_DEBUG)
	flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	if (input.shader_model > qhenki::graphics::ShaderModel::SM_5_1)
	{
		std::cerr << "D3D11ShaderCompiler: Shader model not supported" << std::endl;
		return false;
	}

	char s_name[256] = {};
	if (constexpr size_t max_length = 256; input.entry_point.size() < max_length)
	{
		std::wcstombs(s_name, input.entry_point.data(), max_length - 1);
	}
	else
	{
		std::cerr << "D3D11ShaderCompiler: Entry point name is too long" << std::endl;
	}

	std::vector<D3D_SHADER_MACRO> macros;
	macros.reserve(input.defines.size() + 1);
	
	// convert the define into D3D_SHADER_MACRO
    for (const auto& define : input.defines)
    {
        // convert the define into D3D_SHADER_MACRO
        // split the string at the first '='
        std::string define_str(define.begin(), define.end());
        size_t pos = define_str.find('=');
        if (pos != std::string::npos)
        {
            macros.push_back({ define_str.substr(0, pos).c_str(), define_str.substr(pos + 1).c_str() });
        }
        else
        {
            macros.push_back({ define_str.c_str(), nullptr });
        }
    }
    macros.push_back({ nullptr, nullptr });

	ComPtr<ID3DBlob> shader_blob = nullptr;
	// TODO: d3dcompiler_47.dll should be linked with the application
	// TODO: custom include handler
	HRESULT hr = D3DCompileFromFile(
		input.path.c_str(),
		macros.data(),
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		s_name,
		D3DHelper::get_shader_model_char(input.shader_type, input.shader_model),
		flags,
		0,
		&shader_blob,
		&error_blob);
	if (FAILED(hr))
	{
		if (error_blob)
		{
			std::wcerr << "D3D11ShaderCompiler: Failed to compile shader :: " << input.path << std::endl;
			std::cerr << (char*)error_blob->GetBufferPointer() << std::endl;
		}
		return false;
	}

	output.internal_state = mkS<ComPtr<ID3DBlob>>(shader_blob);
	output.shader_size = shader_blob->GetBufferSize();
	output.shader_data = shader_blob->GetBufferPointer();

	return true;
}
