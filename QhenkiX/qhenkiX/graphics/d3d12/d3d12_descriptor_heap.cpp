#include "d3d12_descriptor_heap.h"

#include <cassert>

#include "qhenkiX/RHI/descriptor_table.h"

using namespace qhenki::gfx;

bool D3D12DescriptorHeap::create(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_DESC& desc)
{
	this->m_desc = desc;

	if (FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap))))
	{
		OutputDebugStringA("Qhenki D3D12 ERROR: Failed to create descriptor heap\n");
		return false;
	}

	m_descriptor_size = device->GetDescriptorHandleIncrementSize(desc.Type);

	return true;
}

bool D3D12DescriptorHeap::allocate(UINT64* alloc_offset)
{
	std::scoped_lock lock(m_mutex);
	// Check free list
	if (!m_free_list.empty())
	{
		*alloc_offset = m_free_list.back();
		m_free_list.pop_back();
		return true;
	}
	if (m_pointer >= m_desc.NumDescriptors)
	{
		OutputDebugStringA("Qhenki D3D12 ERROR: Failed to allocate descriptor, out of memory in heap\n");
		return false;
	}
	// New descriptor
	*alloc_offset = m_pointer++ * m_descriptor_size;
	return true;
}

void D3D12DescriptorHeap::deallocate(UINT64* alloc_offset)
{
	if (*alloc_offset == CREATE_NEW_DESCRIPTOR)
	{
		OutputDebugStringA("Qhenki D3D12 ERROR: Attempted to deallocate a descriptor that was never allocated\n");
		return;
	}
	std::scoped_lock lock(m_mutex);
	m_free_list.push_back(*alloc_offset);
	*alloc_offset = CREATE_NEW_DESCRIPTOR;
}

unsigned D3D12DescriptorHeap::descriptor_count_to_bytes(unsigned count) const
{
	return count * m_descriptor_size;
}

void D3D12DescriptorHeap::get_CPU_descriptor(D3D12_CPU_DESCRIPTOR_HANDLE* handle, size_t offset_bytes,
                                             size_t num_descriptor_offset) const
{
	*handle = m_heap->GetCPUDescriptorHandleForHeapStart();
	handle->ptr += offset_bytes + num_descriptor_offset * m_descriptor_size;
}

bool D3D12DescriptorHeap::get_GPU_descriptor(D3D12_GPU_DESCRIPTOR_HANDLE* handle, size_t offset_bytes, size_t num_descriptor_offset) const
{
	if (m_desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
	{
		*handle = m_heap->GetGPUDescriptorHandleForHeapStart();
		handle->ptr += offset_bytes + num_descriptor_offset * m_descriptor_size;
		return true;
	}
	OutputDebugStringA("Qhenki D3D12 ERROR: Failed to get GPU start for non shader visible heap\n");
	return false;
}
