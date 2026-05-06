#pragma once

#include "queue.h"

namespace qhenki::gfx
{
struct CommandPool
{
    QueueType queue_type;
    sPtr<void> internal_state;
};
} // namespace qhenki::gfx
