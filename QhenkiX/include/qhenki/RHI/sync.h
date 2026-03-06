#pragma once

namespace qhenki::gfx
{
struct Fence
{
    sPtr<void> internal_state;
};
struct WaitInfo
{
    bool wait_all; // If false will wait on any of the fences
    unsigned count;
    Fence* fences;
    uint64_t* values;
    uint64_t timeout = std::numeric_limits<uint64_t>::max();
};
} // namespace qhenki::gfx
