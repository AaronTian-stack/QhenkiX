#pragma once

#include <limits>
#include "descriptor_heap.h"

namespace qhenki::gfx
{
struct DescriptorTable;

struct DescriptorTableDesc
{
    size_t offset; // Offset from start of heap in bytes
    size_t count;  // Number of descriptors in this table
    DescriptorHeap* heap;
};

// Creates a new descriptor in the heap, otherwise use the already existing offset to recreate the descriptor
constexpr size_t CREATE_NEW_DESCRIPTOR = std::numeric_limits<size_t>::max();

struct Descriptor
{
    DescriptorHeap* heap = nullptr;
    // Offset into heap in bytes, or offset into list of views for compatibility mode
    size_t offset = CREATE_NEW_DESCRIPTOR;
    enum Type : uint8_t
    {
        BUFFER,
        TEXTURE,
        SAMPLER,
    } type;
};

struct DescriptorTable
{
    DescriptorTableDesc desc;
    Descriptor get_start_descriptor() const
    {
        return {desc.heap, desc.offset};
    }
};

enum class BufferDescriptorType
{
    CBV,
    UAV,
};
} // namespace qhenki::gfx
