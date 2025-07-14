#pragma once
#include <optional>
#include <string>
#include <vector>

namespace qhenki::util
{
	struct FileHelper
	{
		/**
		 * Reads a file into memory.
         * @param path Wide string path to read from 
		 * @param data (out) Pointer to the data read from the file. Caller responsible for freeing the memory.
		 * @param size (out) Size in bytes of the data read from the file.
		 * @return Whether the file was successfully read or not. Could fail to failing to open or read.
         */
        static bool read_file(const wchar_t* path, void** data, size_t* size);
        static bool write_file(const wchar_t* path, const void* data, size_t size);

		/**
		 * Read a file into memory.
		 * @param path String path to read from
		 * @param data (out) Pointer to the data read from the file. Caller responsible for freeing the memory.
		 * @param size (out) Size in bytes of the data read from the file.
		 * @return Whether the file was successfully read or not. Could fail to failing to open or read.
         */
        static bool read_file(const char* path, void** data, size_t* size);
        static bool write_file(const char* path, const void* data, size_t size);
	};
}