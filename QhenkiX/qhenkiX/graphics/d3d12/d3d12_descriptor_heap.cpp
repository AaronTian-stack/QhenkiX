#include "d3d12_descriptor_heap.h"

using namespace qhenki::gfx;

bool D3D12DescriptorHeap::create(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_DESC& desc)
{
	this->m_desc_ = desc;

	if (FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap_))))
	{
		OutputDebugString(L"Qhenki D3D12: Failed to create descriptor heap\n");
		return false;
	}

	m_descriptor_size_ = device->GetDescriptorHandleIncrementSize(desc.Type);

	D3D12MA::VIRTUAL_BLOCK_DESC block_desc = {};
	block_desc.Size = desc.NumDescriptors * m_descriptor_size_;
	if (FAILED(CreateVirtualBlock(&block_desc, &m_block_)))
	{
		OutputDebugString(L"Qhenki D3D12: Failed to create virtual block\n");
		return false;
	}

	return true;
}

bool D3D12DescriptorHeap::allocate(D3D12MA::VirtualAllocation& alloc, UINT64& alloc_offset, size_t descriptor_count)
{
	D3D12MA::VIRTUAL_ALLOCATION_DESC desc{};
	desc.Size = descriptor_count * m_descriptor_size_;
	desc.Alignment = m_descriptor_size_;

	std::scoped_lock lock(m_block_mutex_);
	if (FAILED(m_block_->Allocate(&desc, &alloc, &alloc_offset)))
	{
		OutputDebugString(L"Qhenki D3D12: Failed to get offset from virtual allocation\n");
		return false;
	}
	return true;
}

bool D3D12DescriptorHeap::get_CPU_descriptor(D3D12_CPU_DESCRIPTOR_HANDLE& handle, size_t num_descriptor_offset)
{
	if (m_desc_.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
	{
		OutputDebugString(L"Qhenki D3D12: Cannot get CPU start for shader visible heap\n");
		return false;
	}
	handle = m_heap_->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += num_descriptor_offset * m_descriptor_size_;
	return true;
}
