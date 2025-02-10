#pragma once
#include <string>
#include <vector>

#include "graphics/qhenki/shader.h"

struct CompilerInput
{
	// TODO: flags
	qhenki::graphics::ShaderType shader_type;
	std::wstring path;
	std::wstring entry_point = L"main";
	qhenki::graphics::ShaderModel shader_model;
	// TODO: includes
	std::vector<std::wstring> defines;
};

struct CompilerOutput
{
	sPtr<void> internal_state;
	size_t shader_size;
	const void* shader_data;
	// DXC shaders also need to be signed by dxil.dll
};

class ShaderCompiler
{
public:
	// Creates a source blob (DXIL or SPIR-V) from the input
	virtual bool compile(const CompilerInput& input, CompilerOutput& output) = 0;
	virtual ~ShaderCompiler() = default;
};

