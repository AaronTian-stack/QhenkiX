#pragma once

#include <cstdint>

#include "command_list.h"
#include "sync.h"

namespace qhenki::gfx
{
struct SubmitInfo
{
    uint32_t command_list_count;
    CommandList* command_lists;

    uint32_t wait_fence_count;
    Fence* wait_fences;
    uint64_t* wait_values;
    QueueType* wait_queues;

    uint32_t signal_fence_count;
    Fence* signal_fences;
    uint64_t* signal_values;
};
} // namespace qhenki::gfx
