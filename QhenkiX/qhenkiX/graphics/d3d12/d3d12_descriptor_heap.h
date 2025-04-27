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

		std::mutex m_mutex_;

		size_t m_pointer_ = 0;
		std::vector<unsigned> m_free_list_;

	public:
		const D3D12_DESCRIPTOR_HEAP_DESC& desc = m_desc_;
		const UINT& descriptor_size = m_descriptor_size_;
		bool create(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_DESC& desc);

		bool allocate(UINT64* alloc_offset);
		void deallocate(UINT64* alloc_offset);

		bool get_CPU_descriptor(D3D12_CPU_DESCRIPTOR_HANDLE* handle, size_t offset_bytes, size_t num_descriptor_offset) const;
		bool get_GPU_descriptor(D3D12_GPU_DESCRIPTOR_HANDLE* handle, size_t offset_bytes, size_t num_descriptor_offset) const;
		const ComPtr<ID3D12DescriptorHeap>& Get() const
		{
			return m_heap_;
		}
	};
}