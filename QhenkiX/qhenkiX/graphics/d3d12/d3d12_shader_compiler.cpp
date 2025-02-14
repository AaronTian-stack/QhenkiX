#include "d3d12_shader_compiler.h"

#include <cassert>
#include <d3dcompiler.h>
#include <stdexcept>

#include "graphics/shared/d3d_helper.h"
#include "graphics/shared/filehelper.h"

DXGI_FORMAT D3D12ShaderCompiler::mask_to_format(const uint32_t mask)
{
	switch (mask)
	{
	case 0x1:
		return DXGI_FORMAT_R32_UINT;
	case 0x3:
		return DXGI_FORMAT_R32G32_UINT;
	case 0x7:
		return DXGI_FORMAT_R32G32B32_UINT;
	case 0xF:
		return DXGI_FORMAT_R32G32B32A32_UINT;
	default:
		throw std::runtime_error("D3D12ShaderCompiler: mask_to_format: mask not implemented");
	}
}

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
	// DXC does not support < SM 6.0
	// Use FXC
	if (input.min_shader_model < qhenki::graphics::ShaderModel::SM_6_0)
	{
		return m_d3d11_shader_compiler_.compile(input, output);
	}

	DxcBuffer source_buffer;
	const auto data = FileHelper::read_file(input.path);
	if (!data.has_value())
	{
		output.error_message = "D3D12ShaderCompiler: Failed to read/open file :: " + std::string(input.path.begin(), input.path.end());
		return false;
	}
	const auto& data_value = data.value();
	source_buffer.Ptr = data_value.data();
	source_buffer.Size = data_value.size();
	source_buffer.Encoding = DXC_CP_ACP;

	// Create default file include handler
	// TODO: custom include handlers
	ComPtr<IDxcIncludeHandler> include_handler;
	if (FAILED(m_library_->CreateDefaultIncludeHandler(&include_handler)))
	{
		output.error_message = "D3D12ShaderCompiler: Failed to create include handler";
		return false;
	}

	std::vector<std::wstring> args;
	args.reserve(input.defines.size() * 2 + 10);

	args.emplace_back(L"-E");
	args.push_back(input.entry_point);
	for (const auto& define : input.defines)
	{
		args.emplace_back(L"-D");
		args.push_back(define);
	}

	// Set target profile
	args.emplace_back(L"-T");
	args.emplace_back(D3DHelper::get_shader_model_wchar(input.shader_type, input.min_shader_model));

	args.emplace_back(L"-Qstrip_debug");
	args.emplace_back(L"-Qstrip_reflect");


#ifdef _DEBUG
	args.emplace_back(DXC_ARG_DEBUG); // debug info
#endif
	args.emplace_back(DXC_ARG_ENABLE_STRICTNESS); // strict mode
	args.emplace_back(DXC_ARG_WARNINGS_ARE_ERRORS); //-WX

	// c_str() version of all args
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
		static_cast<UINT32>(args_ptrs.size()),
		include_handler.Get(),
		IID_PPV_ARGS(&result)))
	{
		// Get any errors
		ComPtr<IDxcBlobUtf8> errors = nullptr;
		result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.GetAddressOf()), nullptr);
		if (errors && errors->GetStringLength())
		{
			output.error_message = errors->GetStringPointer();
		}

		return false;
	}

	output.internal_state = mkS<D3D12ShaderOutput>();
	const auto d3d12_output = static_cast<D3D12ShaderOutput*>(output.internal_state.get());

	// Save the blob in output
	auto hr_s = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(d3d12_output->shader_blob.GetAddressOf()), nullptr);
	assert(SUCCEEDED(hr_s));
	output.shader_size = d3d12_output->shader_blob->GetBufferSize();
	output.shader_data = d3d12_output->shader_blob->GetBufferPointer();

	auto hr_r = result->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(d3d12_output->reflection_blob.GetAddressOf()), nullptr);
	assert(SUCCEEDED(hr_r));
	auto hr_rs = result->GetOutput(DXC_OUT_ROOT_SIGNATURE, IID_PPV_ARGS(d3d12_output->root_signature_blob.GetAddressOf()), nullptr);

	return true;
}

D3D12ShaderCompiler::~D3D12ShaderCompiler()
{
}
