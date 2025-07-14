#include "d3d12_shader_compiler.h"

#include <cassert>
#include <d3dcompiler.h>
#include <stdexcept>

#include "qhenkiX/helper/d3d_helper.h"
#include "qhenkiX/helper/file_helper.h"
#include <filesystem>
#include <utf8.h>

using namespace qhenki::gfx;
using namespace qhenki::util;

DXGI_FORMAT D3D12ShaderCompiler::mask_to_format(const uint32_t mask, const D3D_REGISTER_COMPONENT_TYPE type)
{
	switch (type)
	{
	case D3D_REGISTER_COMPONENT_UNKNOWN:
		throw std::runtime_error("D3D12ShaderCompiler: mask_to_format: Unknown component type");
	case D3D_REGISTER_COMPONENT_UINT32:
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
			throw std::runtime_error("D3D12ShaderCompiler: uint32 mask");
		}
	}
	case D3D_REGISTER_COMPONENT_SINT32:
	{
		switch (mask)
		{
		case 0x1:
			return DXGI_FORMAT_R32_SINT;
		case 0x3:
			return DXGI_FORMAT_R32G32_SINT;
		case 0x7:
			return DXGI_FORMAT_R32G32B32_SINT;
		case 0xF:
			return DXGI_FORMAT_R32G32B32A32_SINT;
		default:
			throw std::runtime_error("D3D12ShaderCompiler: sint32 mask");
		}
	}
	case D3D_REGISTER_COMPONENT_FLOAT32:
	{
		switch (mask)
		{
		case 0x1:
			return DXGI_FORMAT_R32_FLOAT;
		case 0x3:
			return DXGI_FORMAT_R32G32_FLOAT;
		case 0x7:
			return DXGI_FORMAT_R32G32B32_FLOAT;
		case 0xF:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		default:
			throw std::runtime_error("D3D12ShaderCompiler: float32 mask");
	}
		}
	case D3D_REGISTER_COMPONENT_UINT16:
	{
		switch (mask)
		{
		case 0x1:
			return DXGI_FORMAT_R16_UINT;
		case 0x3:
			return DXGI_FORMAT_R16G16_UINT;
		case 0x7:
			throw std::runtime_error("D3D12ShaderCompiler: 3 component uint16 mask");
		case 0xF:
			return DXGI_FORMAT_R16G16B16A16_UINT;
		default:
			throw std::runtime_error("D3D12ShaderCompiler: uint16 mask");
		}
	}
	case D3D_REGISTER_COMPONENT_SINT16:
	{
		switch (mask)
		{
		case 0x1:
			throw std::runtime_error("D3D12ShaderCompiler: 1 component sint16 mask");
		case 0x3:
			return DXGI_FORMAT_R16G16_SINT;
		case 0x7:
			throw std::runtime_error("D3D12ShaderCompiler: 3 component sint16 mask");
		case 0xF:
			return DXGI_FORMAT_R16G16B16A16_SINT;
		}
	}
	case D3D_REGISTER_COMPONENT_FLOAT16:
	{
		switch (mask)
		{
		case 0x1:
			return DXGI_FORMAT_R16_FLOAT;
		case 0x3:
			return DXGI_FORMAT_R16G16_FLOAT;
		case 0x7:
			throw std::runtime_error("D3D12ShaderCompiler: 3 component float16 mask");
		case 0xF:
			return DXGI_FORMAT_R16G16B16A16_FLOAT;
		}
	}
	case D3D_REGISTER_COMPONENT_UINT64:
	case D3D_REGISTER_COMPONENT_SINT64:
	case D3D_REGISTER_COMPONENT_FLOAT64:
			throw std::runtime_error("D3D12ShaderCompiler: 64 bit component type not supported");
	}
	return DXGI_FORMAT_UNKNOWN;
}

D3D12ShaderCompiler::D3D12ShaderCompiler()
{
	if (FAILED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(m_library.ReleaseAndGetAddressOf()))))
	{
		throw std::runtime_error("D3D12ShaderCompiler: Failed to create DxcLibrary");
	}
	if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(m_compiler.ReleaseAndGetAddressOf()))))
	{
		throw std::runtime_error("D3D12ShaderCompiler: Failed to create DxcCompiler");
	}
}

bool D3D12ShaderCompiler::compile(const CompilerInput& input, CompilerOutput& output)
{
	// DXC does not support < SM 6.0, use FXC
	if (input.min_shader_model < ShaderModel::SM_6_0)
	{
		return m_d3d11_shader_compiler.compile(input, output);
	}

	DxcBuffer source_buffer;
	void* data;
	size_t size;
	const auto succeed = FileHelper::read_file(input.path.c_str(), &data, &size);
	if (!succeed)
	{
		output.error_message = "D3D12ShaderCompiler: Failed to read/open file :: " + std::string(input.path.begin(), input.path.end());
		return false;
	}
	source_buffer.Ptr = data;
	source_buffer.Size = size;
	source_buffer.Encoding = DXC_CP_ACP;

	// Create default file include handler
	// TODO: custom include handlers
	ComPtr<IDxcIncludeHandler> include_handler;
	if (FAILED(m_library->CreateDefaultIncludeHandler(&include_handler)))
	{
		output.error_message = "D3D12ShaderCompiler: Failed to create include handler";
		return false;
	}

	static thread_local std::vector<std::wstring> args; // TODO: stack allocator and share with args_ptrs
	args.clear();
	args.reserve((input.defines.size() + input.includes.size()) * 2 + 10);

	if (input.optimization == CompilerInput::Optimization::O0) args.emplace_back(L"-O0");
	if (input.optimization == CompilerInput::Optimization::O1) args.emplace_back(L"-O1");
	if (input.optimization == CompilerInput::Optimization::O2) args.emplace_back(L"-O2");
	// O3 is default

	args.emplace_back(L"-E");
	args.push_back(input.entry_point);
	for (const auto& define : input.defines)
	{
		args.emplace_back(L"-D");
		args.push_back(define);
	}
	for (const auto& include : input.includes)
	{
		args.emplace_back(L"-I");
		std::wstring wstr;
		utf8::utf8to16(include.begin(), include.end(), std::back_inserter(wstr));
		args.push_back(wstr);
	}

	// Set target profile
	args.emplace_back(L"-T");
	args.emplace_back(D3DHelper::get_shader_model_wchar(input.shader_type, input.min_shader_model));

	// TODO: option for stripping reflection data
	args.emplace_back(L"-Qstrip_debug");

	if (input.flags & CompilerInput::DEBUG)
	{
		args.emplace_back(DXC_ARG_DEBUG); // Debug info
	}

	args.emplace_back(DXC_ARG_ENABLE_STRICTNESS); // Strict mode
	args.emplace_back(DXC_ARG_WARNINGS_ARE_ERRORS); //-WX

	// c_str() version of all args
	static thread_local std::vector<const wchar_t*> args_ptrs;
	args_ptrs.clear();
	args_ptrs.reserve(args.size());
	for (const auto& arg : args)
	{
		args_ptrs.push_back(arg.c_str());
	}

	// Compile DXIL blob
	ComPtr<IDxcResult> result;

	auto output_error = [&result, &output]
	{
		// Get any errors
		ComPtr<IDxcBlobUtf8> errors = nullptr;
		result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.ReleaseAndGetAddressOf()), nullptr);
		if (errors && errors->GetStringLength())
		{
			output.error_message = errors->GetStringPointer();
		}
	};

	if FAILED(m_compiler->Compile(
		&source_buffer,
		args_ptrs.data(),
		static_cast<UINT32>(args_ptrs.size()),
		include_handler.Get(),
		IID_PPV_ARGS(&result)))
	{
		output_error();
		return false;
	}

	free(data); // Not needed anymore

	output.internal_state = mkS<D3D12ShaderOutput>();
	const auto d3d12_output = static_cast<D3D12ShaderOutput*>(output.internal_state.get());

	// Save the blob in output
	const auto hr_s = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(d3d12_output->shader_blob.ReleaseAndGetAddressOf()), nullptr);
	if (FAILED(hr_s))
	{
		output_error();
		return false;
	}

	output.shader_size = d3d12_output->shader_blob->GetBufferSize();
	output.shader_data = d3d12_output->shader_blob->GetBufferPointer();

	// Reflection data is part of shader blob
	const auto hr_r = result->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(d3d12_output->reflection_blob.ReleaseAndGetAddressOf()), nullptr);
	if (FAILED(hr_r))
	{
		output_error();
		return false;
	}

	const auto hr_rs = result->GetOutput(DXC_OUT_ROOT_SIGNATURE, IID_PPV_ARGS(d3d12_output->root_signature_blob.ReleaseAndGetAddressOf()), nullptr);
	// The shader might not have a root signature
	if (FAILED(hr_rs))
	{
		d3d12_output->root_signature_blob.Reset();
	}

	if (!input.pdb_path.empty())
	{
		if (!std::filesystem::is_directory(input.pdb_path))
		{
			output.error_message = "D3D12ShaderCompiler: PDB path is not a valid directory :: " + std::string(input.pdb_path.begin(), input.pdb_path.end());
		}
		ComPtr<IDxcBlobUtf16> debug_info_path;
		ComPtr<IDxcBlob> debug_info_blob;
		const auto hr_d = result->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(
			debug_info_blob.ReleaseAndGetAddressOf()),
			debug_info_path.ReleaseAndGetAddressOf());
		// Write debug info to file
		if (SUCCEEDED(hr_d))
		{
			const auto name = debug_info_path->GetStringPointer();

			// TODO: build in stack
            std::wstring path = input.pdb_path;
            if (!path.empty() && path.back() != L'\\' && path.back() != L'/')
            path += L'\\';
            std::wstring pdb_file = path + name;

            auto write_result = FileHelper::write_file(pdb_file.c_str(), debug_info_blob->GetBufferPointer(), debug_info_blob->GetBufferSize());
            assert(write_result);
		}
		else
		{
			output_error();
		}
	}

	return true;
}

D3D12ShaderCompiler::~D3D12ShaderCompiler()
{
}

