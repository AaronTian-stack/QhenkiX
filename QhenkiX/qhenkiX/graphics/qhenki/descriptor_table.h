#pragma once
#include <memory>
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

	struct Descriptor
	{
		DescriptorHeap* heap;
		size_t offset; // Offset into heap in bytes
	};

	struct DescriptorTable
	{
		DescriptorTableDesc desc;
		Descriptor get_start_descriptor() const
		{
			return { desc.heap, desc.offset };
		}
	};
}