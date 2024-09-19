#pragma once

#include <string>

/**
 * Abstraction D3D11 Shader
 */
class Shader
{
public:
	enum type
	{
		vertex, fragment, compute
	};

	std::string source_from_file(const char* filePath);

	void compile_from_source(const char* source, Shader::type type);
};

