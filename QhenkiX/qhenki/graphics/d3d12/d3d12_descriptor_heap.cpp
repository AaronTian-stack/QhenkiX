#include "d3d12_descriptor_heap.h"

#include <cassert>

#include "qhenki/RHI/descriptor.h"

using namespace qhenki::gfx;

bool D3D12DescriptorHeap::create(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_DESC& desc)
{
    this->m_desc = desc;

    if (FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap))))
    {
        OutputDebugStringA("Qhenki D3D12 ERROR: Failed to create descriptor heap\n");
        return false;
    }

    m_descriptor_size = device->GetDescriptorHandleIncrementSize(desc.Type);

    return true;
}

void D3D12DescriptorHeap::get_CPU_descriptor(D3D12_CPU_DESCRIPTOR_HANDLE* handle, const size_t offset_descriptors) const
{
    *handle = m_heap->GetCPUDescriptorHandleForHeapStart();
    handle->ptr += offset_descriptors * m_descriptor_size;
}

bool D3D12DescriptorHeap::get_GPU_descriptor(D3D12_GPU_DESCRIPTOR_HANDLE* handle, const size_t offset_descriptors) const
{
    if (m_desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
    {
        *handle = m_heap->GetGPUDescriptorHandleForHeapStart();
        handle->ptr += offset_descriptors * m_descriptor_size;
        return true;
    }
    OutputDebugStringA("Qhenki D3D12 ERROR: Failed to get GPU start for non shader visible heap\n");
    return false;
}

const ComPtr<ID3D12DescriptorHeap>& D3D12DescriptorHeap::get() const
{
    return m_heap;
}

const D3D12_DESCRIPTOR_HEAP_DESC& D3D12DescriptorHeap::get_desc() const
{
    return m_desc;
}
