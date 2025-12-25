#pragma once

#include <d3dcommon.h>
#include <wrl/client.h>

#include "qhenki/RHI/shader_compiler.h"

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
// https://devblogs.microsoft.com/pix/using-automatic-shader-pdb-resolution-in-pix/
struct ShaderDebugName
{
    uint16_t Flags;      // Reserved, must be set to zero.
    uint16_t NameLength; // Length of the debug name, without null terminator.
    // Followed by NameLength bytes of the UTF-8-encoded name.
    // Followed by a null terminator.
    // Followed by [0-3] zero bytes to align to a 4-byte boundary.
};

struct D3D11ShaderOutput
{
    ComPtr<ID3DBlob> shader_blob;
};

class D3D11ShaderCompiler : public ShaderCompiler
{
public:
    static bool get_compiler_path(char* buffer, size_t length);
    bool get_compiler_path_v(char* buffer, size_t length) override;
    bool compile(const CompilerInput& input, CompilerOutput& output) override;

    friend class D3D12ShaderCompiler;
};
} // namespace qhenki::gfx
