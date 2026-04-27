#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include <variant>

#include "d3d11_descriptor_heap.h"
#include "smartpointer.h"

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
class D3D11SRVUAVHeap : public D3D11DescriptorHeap
{
    using SRV_UAV_Descriptor = std::variant<ComPtr<ID3D11ShaderResourceView>, ComPtr<ID3D11UnorderedAccessView>>;
    uPtr<SRV_UAV_Descriptor[]> m_descriptors;

public:
    bool create(const DescriptorHeapDesc& desc) override;
    void place_srv(size_t offset, ComPtr<ID3D11ShaderResourceView>&& view);
    void place_uav(size_t offset, ComPtr<ID3D11UnorderedAccessView>&& view);
    ID3D11ShaderResourceView* get_srv(size_t offset) const;
    ID3D11UnorderedAccessView* get_uav(size_t offset) const;
    DescriptorHeapDesc::Type heap_type() const override;
};
} // namespace qhenki::gfx
