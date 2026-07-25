#include "dxc_shader_compiler.h"

#include "dxc_include_handler.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <limits>
#include <span>
#include <vector>

#include <SPIRV-Cross/spirv.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#elif defined(__APPLE__) || defined(__linux__)
#include <dlfcn.h>
#include <climits>
#endif

#include <qhenki/memory/arena.h>

#include "qhenki/utility/file_util.h"
#include "qhenki/utility/shader_model_util.h"
#include "qhenki/utility/string_util.h"

using namespace qhenki::gfx;
using namespace qhenki::util;

DXCShaderCompiler::DXCShaderCompiler()
{
    if (FAILED(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(m_library.ReleaseAndGetAddressOf()))))
    {
        fprintf(stderr, "DXCShaderCompiler: Failed to create DxcLibrary\n");
        abort();
    }
    if (FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(m_compiler.ReleaseAndGetAddressOf()))))
    {
        fprintf(stderr, "DXCShaderCompiler: Failed to create DxcCompiler\n");
        abort();
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

bool patch_dxc_read_only_storage_buffers(const void* data,
                                         const size_t size,
                                         std::vector<uint32_t>& patched_words,
                                         std::string& error_message)
{
    constexpr size_t spirv_header_word_count = 5;

    if (data == nullptr || size < spirv_header_word_count * sizeof(uint32_t) || size % sizeof(uint32_t) != 0)
    {
        error_message = "DXCShaderCompiler: DXC returned malformed SPIR-V";
        return false;
    }

    const auto words = std::span(static_cast<const uint32_t*>(data), size / sizeof(uint32_t));
    if (words[0] != SpvMagicNumber)
    {
        error_message = "DXCShaderCompiler: DXC returned an invalid SPIR-V magic number";
        return false;
    }

    const uint32_t id_bound = words[3];
    std::vector<bool> member_is_non_writable(id_bound, false);
    std::vector<bool> variable_is_non_writable(id_bound, false);
    std::vector<uint32_t> storage_buffer_pointer_pointee(id_bound, 0);

    struct StorageBufferVariable
    {
        uint32_t pointer_type;
        uint32_t variable;
    };
    std::vector<StorageBufferVariable> storage_buffer_variables;

    size_t annotation_insert_word = words.size();
    for (size_t word = spirv_header_word_count; word < words.size();)
    {
        const uint16_t word_count = static_cast<uint16_t>(words[word] >> SpvWordCountShift);
        const auto opcode = static_cast<SpvOp>(words[word] & SpvOpCodeMask);
        if (word_count == 0 || word + word_count > words.size())
        {
            error_message = "DXCShaderCompiler: DXC returned malformed SPIR-V instructions";
            return false;
        }

        if (annotation_insert_word == words.size() && opcode >= SpvOpTypeVoid && opcode <= SpvOpTypeForwardPointer)
        {
            annotation_insert_word = word;
        }

        switch (opcode)
        {
        case SpvOpMemberDecorate:
            if (word_count >= 4 && words[word + 1] < id_bound &&
                words[word + 3] == SpvDecorationNonWritable)
            {
                member_is_non_writable[words[word + 1]] = true;
            }
            break;
        case SpvOpDecorate:
            if (word_count >= 3 && words[word + 1] < id_bound &&
                words[word + 2] == SpvDecorationNonWritable)
            {
                variable_is_non_writable[words[word + 1]] = true;
            }
            break;
        case SpvOpTypePointer:
            if (word_count == 4 && words[word + 1] < id_bound &&
                words[word + 2] == SpvStorageClassStorageBuffer)
            {
                storage_buffer_pointer_pointee[words[word + 1]] = words[word + 3];
            }
            break;
        case SpvOpVariable:
            if (word_count >= 4 && words[word + 2] < id_bound &&
                words[word + 3] == SpvStorageClassStorageBuffer)
            {
                storage_buffer_variables.push_back({
                    .pointer_type = words[word + 1],
                    .variable = words[word + 2],
                });
            }
            break;
        default:
            break;
        }

        word += word_count;
    }

    std::vector<uint32_t> variables_to_decorate;
    for (const auto& [pointer_type, variable] : storage_buffer_variables)
    {
        if (pointer_type >= id_bound)
        {
            continue;
        }

        const uint32_t pointee_type = storage_buffer_pointer_pointee[pointer_type];
        if (pointee_type < id_bound && member_is_non_writable[pointee_type] && !variable_is_non_writable[variable])
        {
            variables_to_decorate.push_back(variable);
        }
    }

    if (variables_to_decorate.empty())
    {
        return true;
    }
    if (annotation_insert_word == words.size())
    {
        error_message = "DXCShaderCompiler: Cannot locate the SPIR-V annotation section";
        return false;
    }

    patched_words.reserve(words.size() + variables_to_decorate.size() * 3);
    patched_words.insert(patched_words.end(), words.begin(), words.begin() + annotation_insert_word);
    for (const uint32_t variable : variables_to_decorate)
    {
        patched_words.push_back((3u << SpvWordCountShift) | SpvOpDecorate);
        patched_words.push_back(variable);
        patched_words.push_back(SpvDecorationNonWritable);
    }
    patched_words.insert(patched_words.end(), words.begin() + annotation_insert_word, words.end());
    return true;
}
} // namespace

bool DXCShaderCompiler::get_compiler_path(char* buffer, const size_t length)
{
    if (buffer == nullptr || length == 0)
    {
        return false;
    }
#if defined(_WIN32) || defined(_WIN64)
    if (const auto dx_compiler = GetModuleHandleA(get_dxc_library_name()))
    {
        return GetModuleFileNameA(dx_compiler, buffer, static_cast<DWORD>(length)) != 0;
    }
    return false;
#elif defined(__APPLE__) || defined(__linux__)
    Dl_info info{};
    if (dladdr(reinterpret_cast<const void*>(&DxcCreateInstance), &info) == 0 || info.dli_fname == nullptr)
    {
        return false;
    }

    assert(length >= PATH_MAX);
    if (realpath(info.dli_fname, buffer))
    {
        return true;
    }

    return false;
#else
    return false;
#endif
}

bool DXCShaderCompiler::compile(const CompilerInput& input, CompilerOutput& output, const bool output_spirv)
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

    DxcIncludeHandlerForCompile include_handler(m_library.Get(), input_path, input.includes);

    thread_local memory::Arena arena{4 * MEGABYTE};
    arena.reset();

    const auto args = arena.alloc_array<const wchar_t*>(input.get_defines().size() + input.includes.size() * 2 + 32);
    size_t args_idx = 0;

    if (!args)
    {
        output.error_message = "DXCShaderCompiler: Failed to allocate scratch space";
        return false;
    }

    switch (input.optimization)
    {
    case CompilerInput::Optimization::O0:
        args[args_idx++] = L"-O0";
        break;
    case CompilerInput::Optimization::O1:
        args[args_idx++] = L"-O1";
        break;
    case CompilerInput::Optimization::O2:
        args[args_idx++] = L"-O2";
        break;
    case CompilerInput::Optimization::O3:
        // O3 is default
        break;
    }

    // DXIL libraries don't require an entry point
    std::wstring w_entry_point;
    const bool is_library = input.shader_type == LIBRARY_SHADER;
    if (!is_library)
    {
        args[args_idx++] = L"-E";
        utf8::unchecked::utf8to16(input.entry_point.begin(),
                                  input.entry_point.end(),
                                  std::back_inserter(w_entry_point)); // Hopefully does not cause heap allocation
        args[args_idx++] = w_entry_point.c_str();
    }

    // Upper bound don't know how many characters are actually needed
    constexpr size_t w_buffer_size = 32ull * 1024;
    const auto w_buffer = arena.alloc_array<wchar_t>(w_buffer_size);
    size_t w_buffer_p = 0;
    if (!w_buffer)
    {
        output.error_message = "DXCShaderCompiler: Failed to allocate scratch space";
        return false;
    }

    auto widen_and_push = [&](const std::string& str, const wchar_t* flag)
    {
        args[args_idx++] = flag;
        if (w_buffer_p > w_buffer_size)
        {
            output.error_message = "DXCShaderCompiler: Widen buffer overflow";
            return false;
        }
        utf8::unchecked::utf8to16(str.begin(), str.end(), w_buffer + w_buffer_p);
        w_buffer[w_buffer_p + str.size()] = L'\0'; // Null terminate the string
        args[args_idx++] = w_buffer + w_buffer_p;
        w_buffer_p += 1 + str.size(); // Move pointer forward
        return true;
    };

    for (const auto& define : input.get_defines())
    {
        if (!widen_and_push(define, L"-D"))
        {
            return false;
        }
    }
    for (const auto& include : input.includes)
    {
        if (!widen_and_push(include, L"-I"))
        {
            return false;
        }
    }

    // Set target profile
    args[args_idx++] = L"-T";
    const auto sm = shader_model_wchar(input.shader_type, input.shader_model);
    args[args_idx++] = sm.c_str();

    if (output_spirv)
    {
        args[args_idx++] = L"-spirv";
        args[args_idx++] = L"-fspv-preserve-bindings";
        args[args_idx++] = L"-fspv-flatten-resource-arrays";
        args[args_idx++] = L"-fspv-reflect";
        // TODO: non-zero base vertex and instance?
        if (input.shader_type == VERTEX_SHADER)
        {
            args[args_idx++] = L"-fvk-invert-y";
        }
        args[args_idx++] = L"-fvk-use-dx-layout";
        args[args_idx++] = L"-fvk-use-dx-position-w";
        args[args_idx++] = L"-fspv-target-env=vulkan1.3";
        args[args_idx++] = L"-fspv-preserve-bindings";
        args[args_idx++] = L"-fspv-preserve-interface";
    }

    if (input.flags & CompilerInput::DEBUG)
    {
        args[args_idx++] = DXC_ARG_DEBUG; // Generate debug info (/Zi)
    }
    args[args_idx++] = L"-Qstrip_debug";

    args[args_idx++] = DXC_ARG_ENABLE_STRICTNESS;   // Strict mode
    args[args_idx++] = DXC_ARG_WARNINGS_ARE_ERRORS; // -WX

    ComPtr<IDxcResult> result;

    // Only call this when something bad happens for sure
    auto output_error = [&result, &output]
    {
        // Get any errors
        ComPtr<IDxcBlobUtf8> errors;
        if (const auto o_r = result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(errors.ReleaseAndGetAddressOf()), nullptr);
            SUCCEEDED(o_r) && errors->GetStringLength())
        {
            output.error_message = errors->GetStringPointer();
        }
        else
        {
            output.error_message = "DXCShaderCompiler: Unknown compilation error";
        }
    };

    if (FAILED(m_compiler->Compile(&source_buffer,
                                   args,
                                   static_cast<UINT32>(args_idx),
                                   &include_handler,
                                   IID_PPV_ARGS(result.ReleaseAndGetAddressOf()))))
    {
        output_error();
        return false;
    }

    HRESULT status = S_OK;
    if (FAILED(result->GetStatus(&status)) || FAILED(status))
    {
        output_error();
        return false;
    }

    free(data); // Not needed anymore

    // Save the blob in output
    const auto hr_s = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&output.blob), nullptr);
    if (FAILED(hr_s))
    {
        output_error();
        return false;
    }

    // Workaround for https://github.com/microsoft/DirectXShaderCompiler/issues/8492:
    // DXC currently marks a read-only StructuredBuffer by decorating its block
    // member NonWritable, but VK_EXT_descriptor_heap classifies the resource
    // using NonWritable on the OpVariable. Mirror the decoration onto the
    // StorageBuffer variable until DXC emits the required form itself.
    if (output_spirv)
    {
        std::vector<uint32_t> patched_words;
        if (!patch_dxc_read_only_storage_buffers(output.blob->GetBufferPointer(),
                                                 output.blob->GetBufferSize(),
                                                 patched_words,
                                                 output.error_message))
        {
            return false;
        }

        if (!patched_words.empty())
        {
            const size_t patched_size = patched_words.size() * sizeof(uint32_t);
            if (patched_size > std::numeric_limits<uint32_t>::max())
            {
                output.error_message = "DXCShaderCompiler: Patched SPIR-V exceeds the DXC blob size limit";
                return false;
            }

            ComPtr<IDxcBlobEncoding> patched_blob;
            if (FAILED(m_library->CreateBlob(patched_words.data(),
                                             static_cast<uint32_t>(patched_size),
                                             DXC_CP_ACP,
                                             patched_blob.ReleaseAndGetAddressOf())))
            {
                output.error_message = "DXCShaderCompiler: Failed to create the patched SPIR-V blob";
                return false;
            }
            output.blob->Release();
            output.blob = patched_blob.Detach();
        }
    }

    // Assumed to be null terminated
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
            const auto char_count = debug_info_path->GetBufferSize() / sizeof(wchar_t);

            constexpr wchar_t separator = std::filesystem::path::preferred_separator;
            const size_t required_size = input.pdb_path.size() + char_count +
                                         2; // +1 for separator, +1 for null terminator

            const auto path_buffer = arena.alloc_array<wchar_t>(required_size);
            size_t path_idx = 0;
            if (!path_buffer)
            {
                output.error_message = "DXCShaderCompiler: Failed to allocate scratch space";
                return false;
            }

            // Copy pdb_path
            std::ranges::copy(input.pdb_path, path_buffer + path_idx);
            path_idx += input.pdb_path.size();

            // Add separator if needed
            if (!input.pdb_path.empty() && input.pdb_path.back() != L'\\' && input.pdb_path.back() != L'/')
            {
                path_buffer[path_idx++] = separator;
            }

            // Copy name
            std::ranges::copy(std::span(name, char_count), path_buffer + path_idx);
            path_idx += char_count;

            path_buffer[path_idx] = L'\0';

            const std::wstring_view pdb_file(path_buffer, path_idx);

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
