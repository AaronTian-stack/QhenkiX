#include "d3d12_heap.h"

#include <iostream>

bool D3D12Heap::create(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type)
{
	type_ = type;

	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = 
	{
		.Type = type,
		.NumDescriptors = descriptor_count,
		.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		.NodeMask = 0, // 0 for single adapter
	};

	if (FAILED(device->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap_))))
	{
		std::cerr << "D3D12: Failed to create descriptor heap" << std::endl;
		return false;
	}

	descriptor_size_ = device->GetDescriptorHandleIncrementSize(type);
	return true;
}
