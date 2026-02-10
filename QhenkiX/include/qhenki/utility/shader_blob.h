#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace qhenki::util
{
// [ShaderBlobHeader]
// [Entry0][Define0_0\0][Define0_1\0]...[EntryN][DefineN_*\0]
// [Shader0Bytecode][Shader1Bytecode]...[ShaderNBytecode]

struct ShaderBlobHeader
{
    uint32_t magic;
    uint32_t version;
    uint64_t shader_count;
};

struct ShaderBlobEntry
{
    uint64_t offset; // Offset from start of blob to shader bytecode
    uint64_t size;
    uint32_t define_count; // Number of null terminated define strings following this entry
};

constexpr uint32_t SHADER_BLOB_MAGIC = 0x53584342; // 'SXCB'
constexpr uint32_t SHADER_BLOB_VERSION = 1;

/**
 * Finds a shader permutation in a loaded shader blob.
 * @param blob Pointer to loaded shader blob (e.g. .dxil_blob)
 * @param blob_size Size of the blob in bytes
 * @param defines Array of strings to match (e.g. "NAME=VALUE", "NAME")
 * @param define_count Number of defines in the array
 * @param out_shader (out) Pointer to the found binary data
 * @param out_size (out) Size of the found binary data
 * @return Whether a matching permutation was found successfully.
 */
inline bool find_permutation_in_blob(void* blob,
                                     const size_t blob_size,
                                     const char* const* defines,
                                     const uint32_t define_count,
                                     void** const out_shader,
                                     size_t* out_size)
{
    if (!blob || blob_size < sizeof(ShaderBlobHeader) || !out_shader || !out_size)
    {
        return false;
    }
    auto* base = static_cast<std::byte*>(blob);
    const auto* end = base + blob_size;

    const auto* header = reinterpret_cast<const ShaderBlobHeader*>(base);
    if (header->magic != SHADER_BLOB_MAGIC || header->version != SHADER_BLOB_VERSION)
    {
        return false;
    }

    if (!defines || define_count == 0)
    {
        return false;
    }

    for (uint32_t i = 0; i < define_count; i++)
    {
        if (!defines[i])
        {
            return false;
        }
    }

    // Iterate entries and define strings
    const char* cursor = reinterpret_cast<const char*>(base + sizeof(ShaderBlobHeader));
    for (uint64_t i = 0; i < header->shader_count; i++)
    {
        if (cursor + sizeof(ShaderBlobEntry) > reinterpret_cast<const char*>(end))
        {
            return false;
        }

        const auto* entry = reinterpret_cast<const ShaderBlobEntry*>(cursor);
        cursor += sizeof(ShaderBlobEntry);

        // Get pointer to next entry
        const char* entry_cursor = cursor;
        for (uint32_t d = 0; d < entry->define_count; d++)
        {
            const char* p = entry_cursor;
            while (p < reinterpret_cast<const char*>(end) && *p != '\0')
            {
                p++;
            }
            if (p >= reinterpret_cast<const char*>(end))
            {
                return false;
            }
            entry_cursor = p + 1;
        }

        // Check if all requested defines are present in entry defines
        bool match = true;
        for (uint32_t j = 0; j < define_count; j++)
        {
            const char* req = defines[j];
            const auto req_len = std::strlen(req);

            bool found = false;
            const char* check_cursor = cursor;
            for (uint32_t d = 0; d < entry->define_count; d++)
            {
                // Get the define string from the blob
                const char* start = check_cursor;
                const char* p = start;
                while (p < reinterpret_cast<const char*>(end) && *p != '\0')
                {
                    p++;
                }
                if (p >= reinterpret_cast<const char*>(end))
                {
                    return false;
                }
                const auto len = static_cast<size_t>(p - start);

                if (len == req_len && std::strncmp(start, req, req_len) == 0)
                {
                    found = true;
                    break;
                }
                check_cursor = p + 1;
            }
            // Requested define not found
            if (!found)
            {
                match = false;
                break;
            }
        }

        cursor = entry_cursor;

        if (!match)
        {
            // Not in this shader
            continue;
        }
        if (entry->offset > blob_size)
        {
            return false;
        }

        const auto end_off = entry->offset + entry->size;
        if (end_off > blob_size || end_off < entry->offset)
        {
            return false;
        }

        *out_shader = reinterpret_cast<void*>(base + entry->offset);
        *out_size = static_cast<size_t>(entry->size);

        return true;
    }

    return false;
}
} // namespace qhenki::util
