#pragma once

#include <smartpointer.h>

namespace qhenki::gfx
{
	enum QueueType
	{
		GRAPHICS,
		COMPUTE,
		COPY,
	};
	struct Queue
	{
        QueueType type;
		sPtr<void> internal_state;
	};
}
