#pragma once

#include <d3d11.h>
#include <wrl/client.h>

#include "d3d11_descriptor_heap.h"
#include "smartpointer.h"

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
class D3D11SamplerHeap : public D3D11DescriptorHeap
{
    uPtr<ComPtr<ID3D11SamplerState>[]> sampler_states;

public:
    bool create(const DescriptorHeapDesc& desc) override;
    void place_sampler(size_t offset, ComPtr<ID3D11SamplerState>&& sampler);
    ID3D11SamplerState* get_sampler(size_t offset) const;
    DescriptorHeapDesc::Type heap_type() const override;
};
} // namespace qhenki::gfx
