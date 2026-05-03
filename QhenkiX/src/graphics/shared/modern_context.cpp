#include "modern_context.h"

#include "qhenki/utility/math_util.h"

qhenki::memory::Arena& acquire_arena(const uint64_t current_frame)
{
    thread_local qhenki::memory::Arena arena(4 * qhenki::util::MEGABYTE);
    thread_local uint64_t arena_frame = 0;
    if (arena_frame != current_frame)
    {
        arena_frame = current_frame;
        arena.reset();
    }
    return arena;
}
