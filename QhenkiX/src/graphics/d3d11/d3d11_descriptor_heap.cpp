#include "d3d11_descriptor_heap.h"

#include "qhenki/rhi/descriptor.h"

using namespace qhenki::gfx;

bool D3D11DescriptorHeap::create(const DescriptorHeapDesc& desc)
{
    if (desc.type != heap_type())
    {
        return false;
    }
    m_descriptor_count = desc.num_descriptors;
    return true;
}
