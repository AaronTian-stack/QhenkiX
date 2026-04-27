#pragma once

#include <wrl/client.h>
#include <vector>

#include "D3D12MemAlloc.h"

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
class D3D12DescriptorHeap
{
    D3D12_DESCRIPTOR_HEAP_DESC m_desc{};
    ComPtr<ID3D12DescriptorHeap> m_heap;
    UINT m_descriptor_size = 0;

public:
    D3D12DescriptorHeap() = default;
    D3D12DescriptorHeap(const D3D12DescriptorHeap& other) = delete;
    D3D12DescriptorHeap(D3D12DescriptorHeap&& other) = delete;
    D3D12DescriptorHeap& operator=(const D3D12DescriptorHeap& other) = delete;
    ~D3D12DescriptorHeap() = default;

    bool create(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_DESC& desc);

    // Offsets are additional to start of heap in bytes
    void get_CPU_descriptor(D3D12_CPU_DESCRIPTOR_HANDLE* handle, size_t offset_descriptors) const;

    // Offsets are additional to start of heap in bytes
    bool get_GPU_descriptor(D3D12_GPU_DESCRIPTOR_HANDLE* handle, size_t offset_descriptors) const;

    const ComPtr<ID3D12DescriptorHeap>& get() const;
    const D3D12_DESCRIPTOR_HEAP_DESC& get_desc() const;
};
} // namespace qhenki::gfx
