#pragma once
#include "queue.h"

namespace qhenki::gfx
{
	struct CommandPool
	{
		Queue* queue;
		sPtr<void> internal_state;
	};
}
