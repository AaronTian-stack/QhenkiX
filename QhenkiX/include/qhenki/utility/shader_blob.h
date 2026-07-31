#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "qhenki/rhi/shader.h"

namespace qhenki::util
{
// .slang_blob container layout:
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
constexpr uint32_t SHADER_BLOB_VERSION = 3;

// These are "private" utility functions. Since shader_blob.h is a single header file I have to put them here, but you
// should not call these at all
namespace shader_blob_detail
{
inline bool read_entry(const std::byte** cursor,
                       const std::byte* const end,
                       ShaderBlobEntry* const out_entry,
                       const char** const out_defines = nullptr)
{
    auto& cursor_r = *cursor;
    if (cursor_r > end || static_cast<size_t>(end - cursor_r) < sizeof(ShaderBlobEntry))
    {
        return false;
    }

    std::memcpy(out_entry, cursor_r, sizeof(ShaderBlobEntry));
    cursor_r += sizeof(*out_entry);
    if (out_defines)
    {
        *out_defines = reinterpret_cast<const char*>(cursor_r);
    }

    for (uint32_t i = 0; i < out_entry->define_count; i++)
    {
        const auto remaining = static_cast<size_t>(end - cursor_r);
        const auto* const terminator = static_cast<const std::byte*>(std::memchr(cursor_r, '\0', remaining));
        if (!terminator)
        {
            return false;
        }
        cursor_r = terminator + 1;
    }
    return true;
}

inline bool set_shader_view(const gfx::Shader& blob, const ShaderBlobEntry& entry, gfx::Shader* const out_shader)
{
    const auto shader_end = entry.offset + entry.size;
    if (entry.offset > blob.size || shader_end < entry.offset || shader_end > blob.size)
    {
        return false;
    }

    *out_shader = {
        .data = static_cast<std::byte*>(blob.data) + static_cast<size_t>(entry.offset),
        .size = static_cast<size_t>(entry.size),
    };
    return true;
}
} // namespace shader_blob_detail

/**
 * Retrieves native shader bytecode by its index.
 * @param blob Loaded .slang_blob container.
 * @param shader_index Index of the shader payload to retrieve.
 * @param out_shader Receives a non-owning view of the selected shader bytecode.
 * @return Whether the requested shader bytecode was retrieved successfully.
 */
inline bool get_shader_from_blob(const gfx::Shader& blob, const uint64_t shader_index, gfx::Shader* const out_shader)
{
    if (!out_shader || !blob.data || blob.size < sizeof(ShaderBlobHeader))
    {
        return false;
    }

    ShaderBlobHeader header{};
    std::memcpy(&header, blob.data, sizeof(header));
    if (header.magic != SHADER_BLOB_MAGIC || header.version != SHADER_BLOB_VERSION ||
        shader_index >= header.shader_count)
    {
        return false;
    }

    auto* const base = static_cast<std::byte*>(blob.data);
    const auto* const end = base + blob.size;
    const std::byte* cursor = base + sizeof(ShaderBlobHeader);
    ShaderBlobEntry entry{};
    for (uint64_t i = 0; i <= shader_index; i++)
    {
        if (!shader_blob_detail::read_entry(&cursor, end, &entry))
        {
            return false;
        }
    }
    return shader_blob_detail::set_shader_view(blob, entry, out_shader);
}

/**
 * Finds the first shader permutation containing every requested define.
 * @param blob Loaded .slang_blob container.
 * @param defines Array of null terminated define strings to match.
 * @param define_count Number of strings in the defines array.
 * @param out_shader Non-owning view of the matching shader bytecode.
 * @return Whether a matching shader permutation was found successfully.
 */
inline bool find_permutation_in_blob(const gfx::Shader& blob,
                                     const char* const* defines,
                                     const uint32_t define_count,
                                     gfx::Shader* const out_shader)
{
    if (!out_shader || !blob.data || blob.size < sizeof(ShaderBlobHeader) || !defines || define_count == 0)
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

    auto* const base = static_cast<std::byte*>(blob.data);
    const auto* const end = base + blob.size;
    ShaderBlobHeader header{};
    std::memcpy(&header, base, sizeof(header));
    if (header.magic != SHADER_BLOB_MAGIC || header.version != SHADER_BLOB_VERSION)
    {
        return false;
    }

    const std::byte* cursor = base + sizeof(ShaderBlobHeader);
    for (uint64_t i = 0; i < header.shader_count; i++)
    {
        ShaderBlobEntry entry{};
        const char* define_list = nullptr;
        if (!shader_blob_detail::read_entry(&cursor, end, &entry, &define_list))
        {
            return false;
        }

        bool match = true;
        for (uint32_t requested = 0; requested < define_count; requested++)
        {
            const auto requested_size = std::strlen(defines[requested]);
            bool found = false;
            const char* define_cursor = define_list;
            for (uint32_t d = 0; d < entry.define_count; d++)
            {
                const auto size = std::strlen(define_cursor);
                if (size == requested_size && std::memcmp(define_cursor, defines[requested], size) == 0)
                {
                    found = true;
                    break;
                }
                define_cursor += size + 1;
            }
            if (!found)
            {
                match = false;
                break;
            }
        }

        if (match)
        {
            return shader_blob_detail::set_shader_view(blob, entry, out_shader);
        }
    }

    return false;
}
} // namespace qhenki::util
