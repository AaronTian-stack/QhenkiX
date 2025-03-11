#pragma once
#include <memory>
#include "descriptor_heap.h"

namespace qhenki::gfx
{
	struct DescriptorTableDesc
	{
		size_t offset;
		size_t descriptor_count;
		DescriptorHeap* heap;
	};

	struct DescriptorTable
	{
		DescriptorTableDesc desc;
		sPtr<void> internal_state;
	};
}