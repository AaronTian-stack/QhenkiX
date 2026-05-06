#pragma once

#include <wrl/client.h>

#include "qhenki/rhi/shader_compiler.h"

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

class FXCShaderCompiler : public ShaderCompiler
{
public:
    static bool get_compiler_path(char* buffer, size_t length);
    bool compile(const CompilerInput& input, CompilerOutput& output, bool output_spirv = false) override;

    friend class DXCShaderCompiler;
};
} // namespace qhenki::gfx
