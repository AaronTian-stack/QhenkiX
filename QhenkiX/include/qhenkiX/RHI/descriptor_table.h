#pragma once
#include "descriptor_heap.h"

namespace qhenki::gfx
{
	struct DescriptorTable;

	struct DescriptorTableDesc
	{
		size_t offset; // Offset from start of heap in bytes
		size_t count; // Number of descriptors in this table
		DescriptorHeap* heap;
	};

	// Creates a new descriptor in the heap, otherwise use the already existing offset to recreate the descriptor.
	constexpr size_t CREATE_NEW_DESCRIPTOR = static_cast<size_t>(-1);

	struct Descriptor
	{
		DescriptorHeap* heap;
		size_t offset = CREATE_NEW_DESCRIPTOR; // Offset into heap in bytes, or offset in descriptors for compatibility mode.
	};

	struct DescriptorTable
	{
		DescriptorTableDesc desc;
		Descriptor get_start_descriptor() const
		{
			return { desc.heap, desc.offset };
		}
	};

	enum class BufferDescriptorType
	{
		CBV,
		UAV,
	};
}