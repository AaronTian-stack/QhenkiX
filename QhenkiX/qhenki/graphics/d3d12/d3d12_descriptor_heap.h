#pragma once

#include <wrl/client.h>
#include <mutex>
#include <vector>

#include "D3D12MemAlloc.h"

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
class D3D12DescriptorHeap
{
    D3D12_DESCRIPTOR_HEAP_DESC m_desc{};
    UINT m_descriptor_size = 0;
    ComPtr<ID3D12DescriptorHeap> m_heap;

    std::mutex m_mutex;

    size_t m_pointer = 0;
    std::vector<size_t> m_free_list;

public:
    const D3D12_DESCRIPTOR_HEAP_DESC& desc = m_desc;
    const UINT& descriptor_size = m_descriptor_size;
    bool create(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_DESC& desc);

    // Thread safe
    bool allocate(size_t* alloc_offset);
    void deallocate(size_t* alloc_offset);

    /**
     * Converts count number of descriptors into size in bytes
     * @param count number of descriptors
     * @return The size of count descriptors in bytes
     */
    unsigned descriptor_count_to_bytes(unsigned count) const;

    // Offsets are additional to start of heap
    void get_CPU_descriptor(D3D12_CPU_DESCRIPTOR_HANDLE* handle,
                            size_t offset_bytes,
                            size_t num_descriptor_offset) const;

    // Offsets are additional to start of heap
    bool get_GPU_descriptor(D3D12_GPU_DESCRIPTOR_HANDLE* handle,
                            size_t offset_bytes,
                            size_t num_descriptor_offset) const;

    const ComPtr<ID3D12DescriptorHeap>& get() const;
};
} // namespace qhenki::gfx
