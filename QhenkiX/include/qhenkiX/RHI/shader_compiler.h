#pragma once

#include <span>
#include <string>
#include <variant>
#include <vector>

#include "qhenkiX/RHI/shader.h"
#include <qhenkiX/helper/math_helper.h>

struct NonOwning
{
	const std::string_view path;
	std::span<const std::string> defines;
};

struct Owning
{
	std::string path;
	std::vector<std::string> defines;
};

struct CompilerInput
{
	std::variant<NonOwning, Owning> path_and_defines;
	std::string_view pdb_path;
	std::string entry_point = "main";
	std::span<const std::string> includes;
	qhenki::gfx::ShaderModel shader_model;
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

	// TODO: replace with switch statement
	std::string_view get_path() const
	{
		return std::visit([](const auto& p) -> std::string_view
		{
		return p.path;
		}, path_and_defines);
	}

	std::span<const std::string> get_defines() const
	{
		return std::visit([](const auto& p) -> std::span<const std::string>
		{
			return p.defines;
		}, path_and_defines);
	}
};

struct CompilerOutput
{
	std::string error_message;
	size_t shader_size;
	const void* shader_data;
	sPtr<void> internal_state;
	// DXC shaders also need to be signed by dxil.dll
};

class ShaderCompiler
{
public:
	virtual void get_shader_dll_path(char* buffer, size_t buffer_length) = 0;
	// Creates a source blob (DXIL or SPIR-V) from the input
	virtual bool compile(const CompilerInput& input, CompilerOutput& output) = 0;
	virtual ~ShaderCompiler() = default;
};

