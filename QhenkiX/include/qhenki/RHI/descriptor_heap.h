#pragma once

#include "smartpointer.h"

namespace qhenki::gfx
{
struct DescriptorHeapDesc
{
    enum class Type
    {
        CBV_SRV_UAV,
        SAMPLER,
    } type;
    enum class Visibility
    {
        CPU,
        GPU,
    } visibility;
    unsigned num_descriptors;
};

struct DescriptorHeap
{
    DescriptorHeapDesc desc;
    sPtr<void> internal_state;
};
} // namespace qhenki::gfx
