#pragma once
#include <cstdint>

#include "command_list.h"
#include "sync.h"

namespace qhenki::gfx
{
	// vkQueueSubmit2
	struct SubmitInfo
	{
		// TODO: stage mask
		uint32_t command_list_count;
		CommandList* command_lists;
		uint32_t signal_fence_count;
		Fence* signal_fences;
		uint64_t* signal_values;
	};
}
