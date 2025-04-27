#include "d3d12_descriptor_heap.h"

#include <cassert>

using namespace qhenki::gfx;

bool D3D12DescriptorHeap::create(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_DESC& desc)
{
	this->m_desc_ = desc;

	if (FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap_))))
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to create descriptor heap\n");
		return false;
	}

	m_descriptor_size_ = device->GetDescriptorHandleIncrementSize(desc.Type);

	return true;
}

bool D3D12DescriptorHeap::allocate(UINT64* alloc_offset)
{
	// Check free list
	if (!m_free_list_.empty())
	{
		*alloc_offset = m_free_list_.back();
		m_free_list_.pop_back();
		return true;
	}
	if (m_pointer_ >= m_desc_.NumDescriptors)
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to allocate descriptor, out of memory in heap\n");
		return false;
	}
	// New descriptor
	*alloc_offset = m_pointer_++ * m_descriptor_size_;
	return true;
}

void D3D12DescriptorHeap::deallocate(UINT64* alloc_offset)
{
	assert(false);
}

bool D3D12DescriptorHeap::get_CPU_descriptor(D3D12_CPU_DESCRIPTOR_HANDLE* handle, size_t offset_bytes, size_t num_descriptor_offset) const
{
	if (m_desc_.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Failed to get CPU start for shader visible heap\n");
		return false;
	}
	*handle = m_heap_->GetCPUDescriptorHandleForHeapStart();
	handle->ptr += offset_bytes + num_descriptor_offset * m_descriptor_size_;
	return true;
}

bool D3D12DescriptorHeap::get_GPU_descriptor(D3D12_GPU_DESCRIPTOR_HANDLE* handle, size_t offset_bytes, size_t num_descriptor_offset) const
{
	if (m_desc_.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
	{
		*handle = m_heap_->GetGPUDescriptorHandleForHeapStart();
		handle->ptr += offset_bytes + num_descriptor_offset * m_descriptor_size_;
		return true;
	}
	OutputDebugString(L"Qhenki D3D12 ERROR: Failed to get GPU start for non shader visible heap\n");
	return false;
}
