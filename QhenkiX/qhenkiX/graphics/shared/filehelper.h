#pragma once
#include <optional>
#include <string>
#include <vector>

namespace qhenki::util
{
	class FileHelper
	{
	public:
		// read a file as a vector of bytes
		static std::optional<std::vector<uint8_t>> read_file(const std::wstring& path);
		static bool write_file(const std::wstring& path, const void* data, size_t size);
	};
}