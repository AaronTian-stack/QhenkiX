#pragma once
#include <vector>
#include <wrl/client.h>

#include "D3D12MemAlloc.h"

using Microsoft::WRL::ComPtr;

class D3D12Heap
{
	UINT descriptor_size_ = 0;
	D3D12_DESCRIPTOR_HEAP_TYPE type_;
	std::vector<UINT> free_list_; // TODO: free list

public:
	static inline constexpr UINT descriptor_count = 1000;
	ComPtr<ID3D12DescriptorHeap> heap_;

	const UINT descriptor_size = descriptor_size_;
	const D3D12_DESCRIPTOR_HEAP_TYPE type = type_;

	bool create(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type);
};
