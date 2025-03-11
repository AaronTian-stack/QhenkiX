#pragma once
#include <mutex>
#include <vector>
#include <wrl/client.h>

#include "D3D12MemAlloc.h"

using Microsoft::WRL::ComPtr;

namespace qhenki::gfx
{
class D3D12DescriptorHeap
{
		D3D12_DESCRIPTOR_HEAP_DESC m_desc_{};
		UINT m_descriptor_size_ = 0;
		ComPtr<ID3D12DescriptorHeap> m_heap_;

		std::mutex m_block_mutex_;
		D3D12MA::VirtualBlock* m_block_ = nullptr; // NOT thread safe

public:
		const D3D12_DESCRIPTOR_HEAP_DESC& desc = m_desc_;
		const UINT& descriptor_size = m_descriptor_size_;
	bool create(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_DESC& desc);
		// Reserves a block/table of descriptors. Does not actually create any!
		bool allocate(D3D12MA::VirtualAllocation& alloc, UINT64& alloc_offset, size_t descriptor_count);
		bool get_CPU_descriptor(D3D12_CPU_DESCRIPTOR_HANDLE& handle, size_t num_descriptor_offset);
};
}