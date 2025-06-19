#include "d3d12_descriptor_heap.h"

#include <cassert>

#include "graphics/qhenki/descriptor_table.h"

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
	std::scoped_lock lock(m_mutex_);
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
	if (*alloc_offset == CREATE_NEW_DESCRIPTOR)
	{
		OutputDebugString(L"Qhenki D3D12 ERROR: Attempted to deallocate a descriptor that was never allocated\n");
		return;
	}
	std::scoped_lock lock(m_mutex_);
	m_free_list_.push_back(*alloc_offset);
	*alloc_offset = CREATE_NEW_DESCRIPTOR;
}

unsigned D3D12DescriptorHeap::descriptor_count_to_bytes(unsigned count) const
{
	return count * m_descriptor_size_;
}

bool D3D12DescriptorHeap::get_CPU_descriptor(D3D12_CPU_DESCRIPTOR_HANDLE* handle, size_t offset_bytes, size_t num_descriptor_offset) const
{
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
