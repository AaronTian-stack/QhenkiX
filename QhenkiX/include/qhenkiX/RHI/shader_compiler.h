#pragma once
#include <string>
#include <vector>

#include "qhenkiX/RHI/shader.h"

struct CompilerInput
{
	enum ShaderFlags : uint8_t
	{
		NONE = 0,
		DEBUG = 1 << 1, // Extracts and saves debug PDB
	};

	ShaderFlags flags{};
	qhenki::gfx::ShaderType shader_type;
	std::wstring path;
	std::wstring entry_point = L"main";
	qhenki::gfx::ShaderModel min_shader_model;
	// TODO: custom includes
	std::vector<std::wstring> defines;
};

struct CompilerOutput
{
	sPtr<void> internal_state;
	size_t shader_size;
	const void* shader_data;
	std::string error_message;
	// DXC shaders also need to be signed by dxil.dll
};

class ShaderCompiler
{
public:
	// Creates a source blob (DXIL or SPIR-V) from the input
	virtual bool compile(const CompilerInput& input, CompilerOutput& output) = 0;
	virtual ~ShaderCompiler() = default;
};

