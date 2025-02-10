#pragma once
#include "graphics/qhenki/shader_compiler.h"

class D3D11ShaderCompiler : public ShaderCompiler
{
public:
	bool compile(const CompilerInput& input, CompilerOutput& output) override;
};
