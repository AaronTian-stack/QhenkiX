#include "d3d11_shader_compiler.h"

#include <cassert>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#include "graphics/shared/d3d_helper.h"

using Microsoft::WRL::ComPtr;

bool D3D11ShaderCompiler::compile(const CompilerInput& input, CompilerOutput& output)
{
	ComPtr<ID3DBlob> error_blob = nullptr;
	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;

	// TODO: user flags
	// | D3DCOMPILE_SKIP_OPTIMIZATION;

#if defined(_DEBUG)
	flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_DEBUG_NAME_FOR_SOURCE;
#endif

	if (input.min_shader_model > qhenki::gfx::ShaderModel::SM_5_0)
	{
		output.error_message = "D3D11ShaderCompiler: Shader model not supported";
		return false;
	}

	constexpr size_t max_length = 256;
	char s_name[max_length] = {};
	size_t converted_chars = 0;
	auto error = wcstombs_s(&converted_chars, s_name, input.entry_point.c_str(), max_length - 1);
	if (error != 0)
	{
		output.error_message = "D3D11ShaderCompiler: Failed to convert entry point name from wstring to string";
		return false;
	}

	std::vector<D3D_SHADER_MACRO> macros;
	macros.reserve(input.defines.size() + 1);

	// need to keep in scope for substrings until shader is compiled
	std::vector<std::string> defines;
	defines.reserve(input.defines.size() * 2);
	
	// convert the define into D3D_SHADER_MACRO
    for (const auto& define : input.defines)
    {
        // convert the define into D3D_SHADER_MACRO
        // split the string at the first '='
        std::string define_str(define.begin(), define.end());
        size_t pos = define_str.find('=');
        if (pos != std::string::npos)
        {
			defines.push_back(define_str.substr(0, pos));
			auto& substr1 = defines.back();
			defines.push_back(define_str.substr(pos + 1));
			auto& substr2 = defines.back();
            macros.push_back({ substr1.c_str(), substr2.c_str() });
        }
        else
        {
            macros.push_back({ define_str.c_str(), nullptr });
        }
    }
    macros.push_back({ nullptr, nullptr });

	const auto target = D3DHelper::get_shader_model_char(input.shader_type, input.min_shader_model);
	assert(target);

	ComPtr<ID3DBlob> shader_blob = nullptr;
	// TODO: d3dcompiler_47.dll should be linked with the application
	// TODO: custom include handler
	const HRESULT hr = D3DCompileFromFile(
		input.path.c_str(),
		macros.data(),
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		s_name,
		target,
		flags,
		0,
		&shader_blob,
		&error_blob);
	if (FAILED(hr))
	{
		if (error_blob)
		{
			output.error_message = static_cast<char*>(error_blob->GetBufferPointer());
		}
		return false;
	}

	output.internal_state = mkS<D3D11ShaderOutput>();
	output.shader_size = shader_blob->GetBufferSize();
	output.shader_data = shader_blob->GetBufferPointer();

	const auto d3d_shader_output = static_cast<D3D11ShaderOutput*>(output.internal_state.get());
	d3d_shader_output->shader_blob = shader_blob;

	// Get the root signature
	// The shader might not have a root signature (e.g. D3D11 shaders)
	D3DGetBlobPart(shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(), D3D_BLOB_ROOT_SIGNATURE, 0, 
		d3d_shader_output->root_signature_blob.GetAddressOf());

	// PDB Blob
	D3DGetBlobPart(shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(), D3D_BLOB_PDB, 0, d3d_shader_output->debug_info_blob.GetAddressOf());

	// Get the generated PDB path
	D3DGetBlobPart(shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(), D3D_BLOB_DEBUG_NAME, 0, d3d_shader_output->debug_info_path.GetAddressOf());

	return true;
}
