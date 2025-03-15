#pragma once
#include <memory>
#include "descriptor_heap.h"

namespace qhenki::gfx
{
	struct DescriptorTableDesc
	{
		size_t offset; // In BYTES
		size_t descriptor_count;
		DescriptorHeap* heap;
	};

	struct DescriptorTable
	{
		DescriptorTableDesc desc;
		sPtr<void> internal_state; // Internal allocation info, no reference to heap itself
	};

	struct Descriptor
	{
		DescriptorTable* table;
		size_t descriptor_offset; // Offset into table (in descriptors)
	};
}