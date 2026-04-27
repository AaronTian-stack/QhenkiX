#include "d3d11_srv_uav_heap.h"

#include <cassert>

using namespace qhenki::gfx;

bool D3D11SRVUAVHeap::create(const DescriptorHeapDesc& desc)
{
    if (!D3D11DescriptorHeap::create(desc))
    {
        return false;
    }
    m_descriptors = mkU<SRV_UAV_Descriptor[]>(desc.num_descriptors);
    return true;
}

void D3D11SRVUAVHeap::place_srv(const size_t offset, ComPtr<ID3D11ShaderResourceView>&& view)
{
    assert(offset < m_descriptor_count);
    m_descriptors[offset] = std::move(view);
}

void D3D11SRVUAVHeap::place_uav(const size_t offset, ComPtr<ID3D11UnorderedAccessView>&& view)
{
    assert(offset < m_descriptor_count);
    m_descriptors[offset] = std::move(view);
}

ID3D11ShaderResourceView* D3D11SRVUAVHeap::get_srv(const size_t offset) const
{
    const auto ptr = std::get_if<ComPtr<ID3D11ShaderResourceView>>(&m_descriptors[offset]);
    return ptr ? ptr->Get() : nullptr;
}

ID3D11UnorderedAccessView* D3D11SRVUAVHeap::get_uav(const size_t offset) const
{
    const auto ptr = std::get_if<ComPtr<ID3D11UnorderedAccessView>>(&m_descriptors[offset]);
    return ptr ? ptr->Get() : nullptr;
}

DescriptorHeapDesc::Type D3D11SRVUAVHeap::heap_type() const
{
    return DescriptorHeapDesc::Type::CBV_SRV_UAV;
}
