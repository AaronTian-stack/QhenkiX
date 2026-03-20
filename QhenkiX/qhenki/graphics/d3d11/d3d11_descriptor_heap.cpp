#include "d3d11_descriptor_heap.h"

#include "qhenki/RHI/descriptor.h"

using namespace qhenki::gfx;

bool D3D11DescriptorHeap::create(const DescriptorHeapDesc& desc)
{
    if (desc.type != heap_type())
    {
        return false;
    }
    m_descriptor_count = desc.size;
    return true;
}

bool D3D11DescriptorHeap::allocate(size_t* alloc_offset)
{
    // Check free list
    if (!m_free_list.empty())
    {
        *alloc_offset = m_free_list.back();
        m_free_list.pop_back();
        return true;
    }

    if (m_pointer > m_descriptor_count)
    {
        return false;
    }
    *alloc_offset = m_pointer++;
    return true;
}

void D3D11DescriptorHeap::deallocate(const size_t alloc_offset)
{
    if (alloc_offset == CREATE_NEW_DESCRIPTOR)
    {
        return;
    }
    m_free_list.push_back(alloc_offset);
}
