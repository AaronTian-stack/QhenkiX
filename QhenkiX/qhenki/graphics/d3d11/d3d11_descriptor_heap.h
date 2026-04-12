#pragma once

#include <vector>

#include "qhenki/RHI/descriptor_heap.h"

namespace qhenki::gfx
{
class D3D11DescriptorHeap
{
protected:
    size_t m_descriptor_count = 0;

public:
    D3D11DescriptorHeap() = default;
    D3D11DescriptorHeap(const D3D11DescriptorHeap& other) = delete;
    D3D11DescriptorHeap(D3D11DescriptorHeap&& other) = delete;
    D3D11DescriptorHeap& operator=(const D3D11DescriptorHeap& other) = delete;
    virtual ~D3D11DescriptorHeap() = default;

    virtual bool create(const DescriptorHeapDesc& desc);

    virtual DescriptorHeapDesc::Type heap_type() const = 0;
};
} // namespace qhenki::gfx
