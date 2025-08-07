#include "d3d11_shader_compiler.h"

#include <cassert>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <array>

#include "qhenkiX/helper/d3d_helper.h"
#include <qhenkiX/helper/file_helper.h>
#include <qhenkiX/utility/include_handlers.h>

#include "qhenkiX/helper/string_helper.h"

using Microsoft::WRL::ComPtr;
using namespace qhenki::gfx;
using namespace qhenki::util;

void D3D11ShaderCompiler::get_shader_dll_path(char* buffer, size_t buffer_length)
{
	if (HMODULE hD3DCompiler = GetModuleHandleA("d3dcompiler_47.dll"))
	{
		GetModuleFileNameA(hD3DCompiler, buffer, buffer_length);
	}
	else
	{
		const char* error_message = "d3dcompiler_47.dll not found";
		assert(buffer_length > strlen(error_message));
		(void)snprintf(buffer, buffer_length, "%s", error_message);
	}
}

bool D3D11ShaderCompiler::compile(const CompilerInput& input, CompilerOutput& output)
{
	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;

	if (input.flags & CompilerInput::DEBUG)
	{
		flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_DEBUG_NAME_FOR_SOURCE;
	}

	if (input.optimization == CompilerInput::Optimization::O0)
	{
		flags |= D3DCOMPILE_OPTIMIZATION_LEVEL0;
	}
	else if (input.optimization == CompilerInput::Optimization::O1)
	{
		flags |= D3DCOMPILE_OPTIMIZATION_LEVEL1;
	}
	else if (input.optimization == CompilerInput::Optimization::O2)
	{
		flags |= D3DCOMPILE_OPTIMIZATION_LEVEL2;
	}
	else if (input.optimization == CompilerInput::Optimization::O3)
	{
		flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
	}

	if (input.shader_model > ShaderModel::SM_5_0)
	{
		output.error_message = "D3D11ShaderCompiler: Shader model not supported";
		return false;
	}

	thread_local std::vector<D3D_SHADER_MACRO> macros; // TODO: replace with stack allocator and share with below temp alloc.
	macros.clear();
	macros.reserve(input.get_defines().size() + 1);

	// Need to keep in scope for substrings until shader is compiled
	thread_local std::vector<std::string> defines;
	defines.clear();
	defines.reserve(input.get_defines().size() * 2);

    for (const auto& define : input.get_defines())
    {
        // Convert the defines into D3D_SHADER_MACRO
        // Split the string at the first '='
        size_t pos = define.find('=');
        if (pos != std::string::npos)
        {
			defines.push_back(define.substr(0, pos));
			auto& substr1 = defines.back();
			defines.push_back(define.substr(pos + 1));
			auto& substr2 = defines.back();
            macros.push_back({ .Name= substr1.c_str(), .Definition= substr2.c_str() });
        }
        else
        {
			defines.push_back(define);
			auto& substr1 = defines.back();
            macros.push_back({ .Name= substr1.c_str(), .Definition= nullptr});
        }
    }
    macros.push_back({ .Name= nullptr, .Definition= nullptr});

	const auto target = D3DHelper::get_shader_model_char(input.shader_type, input.shader_model);

	MultiIncludeHandler handler(input.includes);

	ComPtr<ID3DBlob> shader_blob = nullptr;
	// TODO: d3dcompiler_47.dll should be linked with the application
	// TODO: custom include handler

	ComPtr<ID3DBlob> error_blob;
	Utf8To16Scoped path_buffer(input.get_path());
	const HRESULT hr = D3DCompileFromFile(
		path_buffer.c_str(),
		macros.data(),
		&handler,
		input.entry_point.c_str(),
		target.c_str(),
		flags,
		0,
		shader_blob.ReleaseAndGetAddressOf(),
		error_blob.ReleaseAndGetAddressOf());
	if (FAILED(hr))
	{
		if (error_blob && error_blob->GetBufferSize() > 0)
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
		d3d_shader_output->root_signature_blob.ReleaseAndGetAddressOf());

	if (input.flags & CompilerInput::DEBUG)
	{
		ComPtr<ID3DBlob> debug_info_path;
		ComPtr<ID3DBlob> debug_info_blob;
		// PDB Blob
		const auto pdb_result = D3DGetBlobPart(shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(), 
			D3D_BLOB_PDB, 0, debug_info_blob.ReleaseAndGetAddressOf());
		if (FAILED(pdb_result))
		{
			output.error_message = "D3D11ShaderCompiler: Failed to get PDB blob from shader";
			return false;
		}

		// Generated PDB path
		const auto pdb_path = D3DGetBlobPart(shader_blob->GetBufferPointer(), shader_blob->GetBufferSize(), 
			D3D_BLOB_DEBUG_NAME, 0, debug_info_path.ReleaseAndGetAddressOf());
		if (FAILED(pdb_path))
		{
			output.error_message = "D3D11ShaderCompiler: Failed to get debug name blob from shader";
			return false;
		}

		// Convert ID3DBlob to wstring
		const ShaderDebugName* pDebugNameData = reinterpret_cast<const ShaderDebugName*>(debug_info_path->GetBufferPointer());
		const char* pName = reinterpret_cast<const char*>(pDebugNameData + 1);
		const auto result = FileHelper::write_file(pName,
			debug_info_blob->GetBufferPointer(), 
			debug_info_blob->GetBufferSize());
		assert(result);
	}

	return true;
}
