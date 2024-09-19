#include "shader.h"

#include <stdexcept>
#include <fstream>
#include <sstream>

std::string Shader::source_from_file(const char* filePath)
{
    std::string code;
    std::ifstream file;
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
		file.open(filePath);
		std::stringstream stream;
		stream << file.rdbuf();
		file.close();
		code = stream.str();
	}
	catch (std::ifstream::failure e)
	{
		throw std::runtime_error("Failed to read shader file");
    }
	return code;
}
