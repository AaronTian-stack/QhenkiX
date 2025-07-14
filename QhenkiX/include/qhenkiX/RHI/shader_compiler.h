#pragma once

#include <string>
#include <vector>

#include "qhenkiX/RHI/shader.h"
#include <qhenkiX/helper/math_helper.h>

struct CompilerInput
{
	std::wstring path;
	std::wstring pdb_path;
	std::wstring entry_point = L"main";
	std::vector<std::string> includes;
	std::vector<std::wstring> defines;
	qhenki::gfx::ShaderModel min_shader_model;
	qhenki::gfx::ShaderType shader_type;
	enum ShaderFlags : uint8_t
	{
		NONE = BIT(0),
		DEBUG = BIT(1),
	} flags = NONE;
	enum Optimization : uint8_t
	{
		O0, O1, O2, O3
	} optimization = O3;
};

struct CompilerOutput
{
	std::string error_message;
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

