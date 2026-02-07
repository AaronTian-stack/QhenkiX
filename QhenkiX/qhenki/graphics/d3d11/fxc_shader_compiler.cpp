#include "fxc_shader_compiler.h"

#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <array>
#include <cassert>

#include "qhenki/memory/arena.h"
#include "qhenki/utility/d3d_util.h"
#include "qhenki/utility/file_util.h"
#include "qhenki/utility/include_handlers.h"

#include "qhenki/utility/string_util.h"

using Microsoft::WRL::ComPtr;
using namespace qhenki::gfx;
using namespace qhenki::util;

// Static implementation
bool FXCShaderCompiler::get_compiler_path(char* buffer, size_t length)
{
    if (const HMODULE d3d_compiler = GetModuleHandleA("d3dcompiler_47.dll"))
    {
        GetModuleFileNameA(d3d_compiler, buffer, static_cast<DWORD>(length));
        return true;
    }
    const auto error_message = "d3dcompiler_47.dll not found";
    assert(length > strlen(error_message));
    snprintf(buffer, length, "%s", error_message);
    return false;
}

bool FXCShaderCompiler::get_compiler_path_v(char* buffer, size_t length)
{
    return get_compiler_path(buffer, length);
}

bool FXCShaderCompiler::compile(const CompilerInput& input, CompilerOutput& output)
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
        output.error_message = "FXCShaderCompiler: Shader model not supported";
        return false;
    }

    thread_local memory::Arena arena{4 * MEGABYTE};
    arena.reset();

    const auto macros = arena.alloc_array<D3D_SHADER_MACRO>(input.get_defines().size() + 1);
    size_t macros_idx = 0;

    const auto defines = arena.alloc_array_managed<std::string>(input.get_defines().size() * 2);
    size_t defines_idx = 0;

    if (!macros || !defines)
    {
        output.error_message = "FXCShaderCompiler: Failed to allocate scratch space";
        return false;
    }

    for (const auto& define : input.get_defines())
    {
        // Convert the defines into D3D_SHADER_MACRO
        const auto pos = define.find('=');
        if (pos != std::string::npos)
        {
            defines[defines_idx++] = define.substr(0, pos);
            defines[defines_idx++] = define.substr(pos + 1);

            macros[macros_idx++] = {.Name = defines[defines_idx - 2].c_str(),
                                    .Definition = defines[defines_idx - 1].c_str()};
        }
        else
        {
            defines[defines_idx++] = define;
            macros[macros_idx++] = {.Name = defines[defines_idx - 1].c_str(), .Definition = nullptr};
        }
    }
    macros[macros_idx++] = {.Name = nullptr, .Definition = nullptr};

    const auto target = get_shader_model_char(input.shader_type, input.shader_model);

    MultiIncludeHandler handler(input.includes, input.get_path());

    ComPtr<ID3DBlob> shader_blob;
    // TODO: d3dcompiler_47.dll should be linked with the application
    // TODO: custom include handler

    ComPtr<ID3DBlob> error_blob;
    Utf8To16Scoped path_buffer(input.get_path());
    const HRESULT hr = D3DCompileFromFile(path_buffer.c_str(),
                                          macros,
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

    output.internal_state = mkS<FXCShaderOutput>();
    output.shader_size = shader_blob->GetBufferSize();
    output.shader_data = shader_blob->GetBufferPointer();

    const auto fxc_shader_output = static_cast<FXCShaderOutput*>(output.internal_state.get());
    fxc_shader_output->shader_blob = shader_blob;

    if (input.flags & CompilerInput::DEBUG)
    {
        ComPtr<ID3DBlob> debug_info_path;
        ComPtr<ID3DBlob> debug_info_blob;
        const auto pdb_result = D3DGetBlobPart(shader_blob->GetBufferPointer(),
                                               shader_blob->GetBufferSize(),
                                               D3D_BLOB_PDB,
                                               0,
                                               debug_info_blob.ReleaseAndGetAddressOf());
        if (FAILED(pdb_result))
        {
            output.error_message = "FXCShaderCompiler: Failed to get PDB blob from shader";
            return false;
        }

        // Generated PDB path
        const auto pdb_path = D3DGetBlobPart(shader_blob->GetBufferPointer(),
                                             shader_blob->GetBufferSize(),
                                             D3D_BLOB_DEBUG_NAME,
                                             0,
                                             debug_info_path.ReleaseAndGetAddressOf());
        if (FAILED(pdb_path))
        {
            output.error_message = "FXCShaderCompiler: Failed to get debug name blob from shader";
            return false;
        }

        // Convert ID3DBlob to wstring
        const auto debug_name_data = static_cast<const ShaderDebugName*>(debug_info_path->GetBufferPointer());
        const auto name = reinterpret_cast<const char*>(debug_name_data + 1);
        if (!write_file(name, debug_info_blob->GetBufferPointer(), debug_info_blob->GetBufferSize()))
        {
            output.error_message = "FXCShaderCompiler: Failed to write PDB file :: " + std::string(name);
            return false;
        }
    }

    return true;
}
