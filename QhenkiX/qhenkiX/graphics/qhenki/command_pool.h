#pragma once
#include "queue.h"

namespace qhenki
{
	struct CommandPool
	{
		Queue* queue;
		sPtr<void> internal_state;
	};
}
