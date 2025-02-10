#pragma once
#include "queue.h"

namespace qhenki::graphics
{
	struct CommandPool
	{
		Queue* queue;
		sPtr<void> internal_state;
	};
}
