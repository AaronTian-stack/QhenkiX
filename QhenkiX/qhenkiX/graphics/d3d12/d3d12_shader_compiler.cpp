#include "d3d12_shader_compiler.h"

#include <d3dcompiler.h>
#include <stdexcept>
#include <iostream>

#include "graphics/shared/d3d_helper.h"
#include "graphics/shared/filehelper.h"

D3D12ShaderCompiler::D3D12ShaderCompiler()
{
	if (FAILED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&m_library_))))
	{
		throw std::runtime_error("D3D12ShaderCompiler: Failed to create DxcLibrary");
	}
	if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler_))))
	{
		throw std::runtime_error("D3D12ShaderCompiler: Failed to create DxcCompiler");
	}
}

bool D3D12ShaderCompiler::compile(const CompilerInput& input, CompilerOutput& output)
{
	DxcBuffer source_buffer;
	const auto data = FileHelper::read_file(input.path);
	if (!data.has_value())
	{
		std::wcerr << "D3D12ShaderCompiler: Failed to read/open file :: " << input.path << std::endl;
		return false;
	}
	const auto data_value = data.value();
	source_buffer.Ptr = data_value.data();
	source_buffer.Size = data_value.size();
	source_buffer.Encoding = DXC_CP_ACP;

	// Create default file include handler
	// TODO: custom include handlers
	ComPtr<IDxcIncludeHandler> include_handler;
	if (FAILED(m_library_->CreateDefaultIncludeHandler(&include_handler)))
	{
		std::cerr << "D3D12ShaderCompiler: Failed to create include handler" << std::endl;
		return false;
	}

	std::vector<std::wstring> args;
	args.push_back(L"-E");
	args.push_back(input.entry_point);
	for (const auto& define : input.defines)
	{
		args.push_back(L"-D");
		args.push_back(define);
	}

	// Set target profile
	args.push_back(L"-T");
	args.emplace_back(D3DHelper::get_shader_model_wchar(input.shader_type, input.shader_model));

	std::vector<const wchar_t*> args_ptrs;
	args_ptrs.reserve(args.size());
	for (const auto& arg : args)
	{
		args_ptrs.push_back(arg.c_str());
	}

	// Compile DXIL blob
	ComPtr<IDxcResult> result;
	if FAILED(m_compiler_->Compile(
		&source_buffer,
		args_ptrs.data(),
		args_ptrs.size(),
		include_handler.Get(),
		IID_PPV_ARGS(&result)))
	{
		std::wcerr << "D3D12ShaderCompiler: Failed to compile shader :: " << input.path << std::endl;

		// Get any errors
		ComPtr<IDxcBlobUtf8> errors = nullptr;
		result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
		if (errors && errors->GetStringLength())
		{
			std::cerr << errors->GetStringPointer() << std::endl;
		}

		return false;
	}

	// Save the blob in output
	ComPtr<IDxcBlob> blob = nullptr;
	result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&blob), nullptr);
	if (blob)
	{
		output.internal_state = mkS<ComPtr<IDxcBlob>>(blob);
		output.shader_size = blob->GetBufferSize();
		output.shader_data = blob->GetBufferPointer();
	}

	return true;
}

D3D12ShaderCompiler::~D3D12ShaderCompiler()
{
}
