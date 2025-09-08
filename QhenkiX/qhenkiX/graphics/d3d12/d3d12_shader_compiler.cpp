#include "d3d12_shader_compiler.h"

#include <cassert>
#include <d3dcompiler.h>
#include <stdexcept>

#include "qhenkiX/helper/d3d_helper.h"
#include "qhenkiX/helper/file_helper.h"
#include <filesystem>

#include "qhenkiX/helper/string_helper.h"

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
	if (input.shader_model < ShaderModel::SM_6_0)
	{
		return D3D11ShaderCompiler::compile(input, output);
	}

	DxcBuffer source_buffer;
	void* data;
	size_t size;
	const auto& input_path = input.get_path();
	const auto succeed = FileHelper::read_file(input_path.data(), &data, &size);
	if (!succeed)
	{
		output.error_message = "D3D12ShaderCompiler: Failed to read/open file :: " + std::string(input_path.begin(), input_path.end());
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

	thread_local std::vector<const wchar_t*> args; // TODO: stack allocator and share with args_ptrs
	args.clear();
	args.reserve((input.get_defines().size() + input.includes.size()) * 2 + 10);

	if (input.optimization == CompilerInput::Optimization::O0) args.emplace_back(L"-O0");
	if (input.optimization == CompilerInput::Optimization::O1) args.emplace_back(L"-O1");
	if (input.optimization == CompilerInput::Optimization::O2) args.emplace_back(L"-O2");
	// O3 is default

	args.emplace_back(L"-E");
	std::wstring w_entry_point;
	utf8::utf8to16(input.entry_point.begin(), input.entry_point.end(), std::back_inserter(w_entry_point)); // Hopefully does not cause heap allocation
	args.push_back(w_entry_point.c_str());

	// Allocate a large buffer of wchar_t. If it overflows, start making wstring (possible heap allocation)
	std::array<wchar_t, 1024> w_buffer;
	ptrdiff_t w_buffer_p = 0;
	std::vector<std::wstring> wstring_backup;

	auto widen_and_push = [&](const std::string& str, const wchar_t* flag)
	{
		assert(str.size() < LLONG_MAX);
		args.emplace_back(flag);
		// Try to widen the string using w_buffer
		if (w_buffer_p + str.size() + 1 < w_buffer.size())
		{
			utf8::utf8to16(str.begin(), str.end(), w_buffer.begin() + w_buffer_p);
			w_buffer[w_buffer_p + str.size()] = L'\0'; // Null-terminate the string
			args.push_back(w_buffer.data() + w_buffer_p);
			w_buffer_p += 1 + str.size(); // Move pointer forward
		}
		else
		{
			wstring_backup.emplace_back();
			auto& wstr = wstring_backup.back();
			wstr.reserve(str.size());
			utf8::utf8to16(str.begin(), str.end(), std::back_inserter(wstr));
			wstr.push_back(L'\0'); // Null-terminate the string
			args.push_back(wstr.c_str());
		}
	};

	for (const auto& define : input.get_defines())
	{
		widen_and_push(define, L"-D");
	}
	for (const auto& include : input.includes)
	{
		widen_and_push(include, L"-I");
	}

	// Set target profile
	args.emplace_back(L"-T");
	const auto sm = D3DHelper::get_shader_model_wchar(input.shader_type, input.shader_model);
	args.emplace_back(sm.c_str());

	// TODO: option for stripping reflection data
	args.emplace_back(L"-Qstrip_debug");

	if (input.flags & CompilerInput::DEBUG)
	{
		args.emplace_back(DXC_ARG_DEBUG); // Debug info
	}

	args.emplace_back(DXC_ARG_ENABLE_STRICTNESS); // Strict mode
	args.emplace_back(DXC_ARG_WARNINGS_ARE_ERRORS); //-WX

	// Compile DXIL blob
	ComPtr<IDxcResult> result;

	auto output_error = [&result, &output]
	{
		// Get any errors
		ComPtr<IDxcBlobUtf8> errors = nullptr;
		if (const auto o_r = result->GetOutput(DXC_OUT_ERRORS, 
			IID_PPV_ARGS(errors.ReleaseAndGetAddressOf()), nullptr); SUCCEEDED(o_r) && errors->GetStringLength())
		{
			output.error_message = errors->GetStringPointer();
		}
	};

	if FAILED(m_compiler->Compile(
		&source_buffer,
		args.data(),
		static_cast<UINT32>(args.size()),
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
	if (const auto hr_s = result->GetOutput(DXC_OUT_OBJECT, 
		IID_PPV_ARGS(d3d12_output->shader_blob.ReleaseAndGetAddressOf()), nullptr); FAILED(hr_s))
	{
		output_error();
		return false;
	}

	output.shader_size = d3d12_output->shader_blob->GetBufferSize();
	output.shader_data = d3d12_output->shader_blob->GetBufferPointer();

	// Reflection data is part of shader blob
	if (const auto hr_r = result->GetOutput(DXC_OUT_REFLECTION, 
		IID_PPV_ARGS(d3d12_output->reflection_blob.ReleaseAndGetAddressOf()), nullptr); FAILED(hr_r))
	{
		output_error();
		return false;
	}

	// The shader might not have a root signature
	if (const auto hr_rs = result->GetOutput(DXC_OUT_ROOT_SIGNATURE, 
		IID_PPV_ARGS(d3d12_output->root_signature_blob.ReleaseAndGetAddressOf()), nullptr); FAILED(hr_rs))
	{
		d3d12_output->root_signature_blob.Reset();
	}

	// Assumed to be null-terminated!
	const auto& pdb_path = input.pdb_path;
	if (!pdb_path.empty())
	{
		if (!std::filesystem::is_directory(pdb_path))
		{
				output.error_message = "D3D12ShaderCompiler: PDB path is not a valid directory :: " + std::string(pdb_path.begin(), pdb_path.end());
		}
		ComPtr<IDxcBlobUtf16> debug_info_path;
		ComPtr<IDxcBlob> debug_info_blob;
		const auto hr_d = result->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(
			debug_info_blob.ReleaseAndGetAddressOf()),
			debug_info_path.ReleaseAndGetAddressOf());
		// Write PDB
		if (SUCCEEDED(hr_d))
		{
			const auto name = debug_info_path->GetStringPointer();

			std::wstring_view pdb_file;
			std::wstring path;

			constexpr auto buffer_count = 1024;
			std::array<wchar_t, buffer_count> path_buffer;
			const auto char_count = debug_info_path->GetBufferSize() / sizeof(wchar_t);
			assert(false); // TODO: expand pdb_path

			// Try using stack buffer first
			bool failed = true;
			if (1 + input.pdb_path.size() + char_count < sizeof(buffer_count))
			{
				if (swprintf(path_buffer.data(), buffer_count, L"%s\\%s", pdb_path.data(), name) >= 0)
				{
					pdb_file = std::wstring_view(path_buffer.data(), wcslen(path_buffer.data()));
					failed = false;
				}
			}
			// wstring fallback (may allocate heap)
			if (failed)
			{
				path.reserve(pdb_path.size() + char_count);

				path.assign(pdb_path.begin(), pdb_path.end());

				if (!path.empty() && path.back() != L'\\' && path.back() != L'/')
				{
					path += L'\\';
				}
				path.append(name, char_count);
				pdb_file = path;
			}

			if (const auto write_result = FileHelper::write_file(
				pdb_file.data(), debug_info_blob->GetBufferPointer(), debug_info_blob->GetBufferSize()); !write_result)
            {
				output.error_message = "D3D12ShaderCompiler: Failed to write PDB file :: " + std::string(pdb_file.begin(), pdb_file.end());
            }
		}
		else
		{
			output_error();
		}
	}

	return true;
}

bool D3D12ShaderCompiler::get_dll_path(char* buffer1, char* buffer2, unsigned long buffer_length)
{
	assert(buffer1);
	assert(buffer2);
	const auto d3d11_result = D3D11ShaderCompiler::get_dll_path(buffer2, buffer_length);
	if (HMODULE hDXCompiler = GetModuleHandleA("dxcompiler.dll"))
	{
		const auto d3d12_result = GetModuleFileNameA(hDXCompiler, buffer1, buffer_length);
		return d3d11_result && d3d12_result;
	}
	return false;
}

D3D12ShaderCompiler::~D3D12ShaderCompiler()
{
}

