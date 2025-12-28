#include "dxc_shader_compiler.h"

#include <d3dcompiler.h>
#include <cassert>
#include <filesystem>
#include <stdexcept>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#elif defined(__APPLE__) || defined(__linux__)
// TODO
#endif

#include "qhenki/utility/d3d_util.h"
#include "qhenki/utility/file_util.h"
#include "qhenki/utility/string_util.h"

using namespace qhenki::gfx;
using namespace qhenki::util;

DXGI_FORMAT DXCShaderCompiler::mask_to_format(const uint32_t mask, const D3D_REGISTER_COMPONENT_TYPE type)
{
    switch (type)
    {
    case D3D_REGISTER_COMPONENT_UNKNOWN:
        throw std::runtime_error("DXCShaderCompiler: mask_to_format: Unknown component type");
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
            throw std::runtime_error("DXCShaderCompiler: uint32 mask");
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
            throw std::runtime_error("DXCShaderCompiler: sint32 mask");
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
            throw std::runtime_error("DXCShaderCompiler: float32 mask");
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
            throw std::runtime_error("DXCShaderCompiler: 3 component uint16 mask");
        case 0xF:
            return DXGI_FORMAT_R16G16B16A16_UINT;
        default:
            throw std::runtime_error("DXCShaderCompiler: uint16 mask");
        }
    }
    case D3D_REGISTER_COMPONENT_SINT16:
    {
        switch (mask)
        {
        case 0x1:
            throw std::runtime_error("DXCShaderCompiler: 1 component sint16 mask");
        case 0x3:
            return DXGI_FORMAT_R16G16_SINT;
        case 0x7:
            throw std::runtime_error("DXCShaderCompiler: 3 component sint16 mask");
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
            throw std::runtime_error("DXCShaderCompiler: 3 component float16 mask");
        case 0xF:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        }
    }
    case D3D_REGISTER_COMPONENT_UINT64:
    case D3D_REGISTER_COMPONENT_SINT64:
    case D3D_REGISTER_COMPONENT_FLOAT64:
        throw std::runtime_error("DXCShaderCompiler: 64 bit component type not supported");
    }
    return DXGI_FORMAT_UNKNOWN;
}

DXCShaderCompiler::DXCShaderCompiler()
{
    if (FAILED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(m_library.ReleaseAndGetAddressOf()))))
    {
        throw std::runtime_error("DXCShaderCompiler: Failed to create DxcLibrary");
    }
    if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(m_compiler.ReleaseAndGetAddressOf()))))
    {
        throw std::runtime_error("DXCShaderCompiler: Failed to create DxcCompiler");
    }
}

namespace
{
const char* get_dxc_library_name()
{
#if defined(_WIN32) || defined(_WIN64)
    return "dxcompiler.dll";
#elif defined(__APPLE__)
    return "libdxcompiler.dylib";
#elif defined(__linux__)
    return "libdxcompiler.so";
#else
#error "Unsupported platform"
#endif
}
} // namespace

bool DXCShaderCompiler::get_compiler_path(char* buffer, size_t length)
{
#if defined(_WIN32) || defined(_WIN64)
    if (const auto dx_compiler = GetModuleHandleA(get_dxc_library_name()))
    {
        return GetModuleFileNameA(dx_compiler, buffer, static_cast<DWORD>(length)) != 0;
    }
    return false;
#elif defined(__APPLE__)
    // TODO
#elif defined(__linux__)
    // TODO
#else
    return false;
#endif
}

bool DXCShaderCompiler::get_compiler_path_v(char* buffer, size_t length)
{
    return DXCShaderCompiler::get_compiler_path(buffer, length);
}

bool DXCShaderCompiler::compile(const CompilerInput& input, CompilerOutput& output)
{
    if (input.shader_model < ShaderModel::SM_6_0)
    {
        return false;
    }

    DxcBuffer source_buffer;
    void* data;
    size_t size;
    const auto& input_path = input.get_path();
    const auto succeed = read_file(input_path.data(), &data, &size);
    if (!succeed)
    {
        output.error_message = "DXCShaderCompiler: Failed to read/open file :: " +
                               std::string(input_path.begin(), input_path.end());
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
        output.error_message = "DXCShaderCompiler: Failed to create include handler";
        return false;
    }

    thread_local std::vector<const wchar_t*> args; // TODO: stack allocator and share with args_ptrs
    args.clear();
    args.reserve((input.get_defines().size() + input.includes.size()) * 2 + 10);

    if (input.optimization == CompilerInput::Optimization::O0)
        args.emplace_back(L"-O0");
    if (input.optimization == CompilerInput::Optimization::O1)
        args.emplace_back(L"-O1");
    if (input.optimization == CompilerInput::Optimization::O2)
        args.emplace_back(L"-O2");
    // O3 is default

    // DXIL libraries don't require an entry point
    std::wstring w_entry_point;
    const bool is_library = (input.shader_type == LIBRARY_SHADER);
    if (!is_library)
    {
        args.emplace_back(L"-E");
        utf8::utf8to16(input.entry_point.begin(),
                       input.entry_point.end(),
                       std::back_inserter(w_entry_point)); // Hopefully does not cause heap allocation
        args.push_back(w_entry_point.c_str());
    }

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
    const auto sm = get_shader_model_wchar(input.shader_type, input.shader_model);
    args.emplace_back(sm.c_str());

    if (input.flags & CompilerInput::DEBUG)
    {
        args.emplace_back(DXC_ARG_DEBUG); // Generate debug info (/Zi)
    }
    args.emplace_back(L"-Qstrip_debug");

    args.emplace_back(DXC_ARG_ENABLE_STRICTNESS);   // Strict mode
    args.emplace_back(DXC_ARG_WARNINGS_ARE_ERRORS); //-WX

    // Compile DXIL blob
    ComPtr<IDxcResult> result;

    auto output_error = [&result, &output]
    {
        // Get any errors
        ComPtr<IDxcBlobUtf8> errors = nullptr;
        if (const auto o_r = result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.ReleaseAndGetAddressOf()), nullptr);
            SUCCEEDED(o_r) && errors->GetStringLength())
        {
            output.error_message = errors->GetStringPointer();
        }
    };

    if FAILED (m_compiler->Compile(&source_buffer,
                                   args.data(),
                                   static_cast<UINT32>(args.size()),
                                   include_handler.Get(),
                                   IID_PPV_ARGS(&result)))
    {
        output_error();
        return false;
    }

    free(data); // Not needed anymore

    output.internal_state = mkS<DXCShaderOutput>();
    const auto dxc_output = static_cast<DXCShaderOutput*>(output.internal_state.get());

    // Save the blob in output
    if (const auto hr_s =
            result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(dxc_output->shader_blob.ReleaseAndGetAddressOf()), nullptr);
        FAILED(hr_s))
    {
        output_error();
        return false;
    }

    output.shader_size = dxc_output->shader_blob->GetBufferSize();
    output.shader_data = dxc_output->shader_blob->GetBufferPointer();

    // Assumed to be null-terminated!
    const auto& pdb_path = input.pdb_path;
    if (!pdb_path.empty())
    {
        if (!std::filesystem::is_directory(pdb_path))
        {
            output.error_message = "DXCShaderCompiler: PDB path is not a valid directory :: " +
                                   std::string(pdb_path.begin(), pdb_path.end());
        }
        ComPtr<IDxcBlobUtf16> debug_info_path;
        ComPtr<IDxcBlob> debug_info_blob;
        const auto hr_d = result->GetOutput(DXC_OUT_PDB,
                                            IID_PPV_ARGS(debug_info_blob.ReleaseAndGetAddressOf()),
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
            if (1 + input.pdb_path.size() + char_count < buffer_count)
            {
                const wchar_t separator = std::filesystem::path::preferred_separator;
                const auto formatted = qhenki::util::format_wstring<buffer_count>(L"%s%c%s", pdb_path.data(), separator, name);
                if (!formatted.truncated)
                {
                    path_buffer = formatted.buffer;
                    pdb_file = std::wstring_view(path_buffer.data(), wcslen(path_buffer.data()));
                    failed = false;
                }
            }
            // wstring fallback (may allocate heap)
            if (failed)
            {
                path.reserve(pdb_path.size() + char_count);

                path.assign(pdb_path.begin(), pdb_path.end());

                const wchar_t separator = std::filesystem::path::preferred_separator;
                if (!path.empty() && path.back() != L'\\' && path.back() != L'/')
                {
                    path += separator;
                }
                path.append(name, char_count);
                pdb_file = path;
            }

            if (const auto write_result =
                    write_file(pdb_file.data(), debug_info_blob->GetBufferPointer(), debug_info_blob->GetBufferSize());
                !write_result)
            {
                output.error_message = "DXCShaderCompiler: Failed to write PDB file :: " +
                                       std::string(pdb_file.begin(), pdb_file.end());
            }
        }
        else
        {
            output_error();
        }
    }

    return true;
}
