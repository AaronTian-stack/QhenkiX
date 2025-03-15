#pragma once
#include "queue.h"

namespace qhenki::gfx
{
				struct CommandPool
				{
					const Queue* queue; // Change Queue* to const Queue*
					sPtr<void> internal_state;
				};
}