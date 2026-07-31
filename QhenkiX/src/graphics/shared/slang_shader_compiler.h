#pragma once

#include <slang.h>

#include <qhenki/rhi/shader_compiler.h>

namespace qhenki::gfx
{
class SlangShaderCompiler final : public ShaderCompiler
{
    SlangSession* m_session = nullptr;

public:
    SlangShaderCompiler();
    SlangShaderCompiler(const SlangShaderCompiler&) = delete;
    SlangShaderCompiler& operator=(const SlangShaderCompiler&) = delete;

    static bool get_compiler_path(char* buffer, size_t length);

    bool compile(const CompilerInput& input, CompilerOutput& output, bool output_spirv = false) override;

    ~SlangShaderCompiler() override;
};
} // namespace qhenki::gfx
