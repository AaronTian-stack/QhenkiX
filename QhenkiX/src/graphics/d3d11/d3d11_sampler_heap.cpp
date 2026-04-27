#include "d3d11_sampler_heap.h"

#include <cassert>

using namespace qhenki::gfx;

bool D3D11SamplerHeap::create(const DescriptorHeapDesc& desc)
{
    if (!D3D11DescriptorHeap::create(desc))
    {
        return false;
    }
    sampler_states = mkU<ComPtr<ID3D11SamplerState>[]>(desc.num_descriptors);
    return true;
}

void D3D11SamplerHeap::place_sampler(const size_t offset, ComPtr<ID3D11SamplerState>&& sampler)
{
    assert(offset < m_descriptor_count);
    sampler_states[offset] = std::move(sampler);
}

ID3D11SamplerState* D3D11SamplerHeap::get_sampler(const size_t offset) const
{
    assert(offset < m_descriptor_count);
    return sampler_states[offset].Get();
}

DescriptorHeapDesc::Type D3D11SamplerHeap::heap_type() const
{
    return DescriptorHeapDesc::Type::SAMPLER;
}
