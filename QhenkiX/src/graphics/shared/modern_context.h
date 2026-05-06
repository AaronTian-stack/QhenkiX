#pragma once

#include <mutex>

#include "descriptor_flush.h"
#include "qhenki/memory/arena.h"
#include "qhenki/rhi/context.h"

namespace qhenki::gfx
{
class ModernContext : public Context
{
protected:
    // For internal swapchain purposes
    unsigned m_swapchain_index = 0;
    std::mutex m_submit_mutex;
    DeferredDescriptorCopier m_descriptor_copier;
};
} // namespace qhenki::gfx

qhenki::memory::Arena& acquire_arena(uint64_t current_frame);
