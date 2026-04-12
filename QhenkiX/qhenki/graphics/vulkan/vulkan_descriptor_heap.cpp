#include "vulkan_descriptor_heap.h"

#include <cassert>
#include "vulkan_macros.h"

using namespace qhenki::gfx;

VulkanDescriptorHeap::~VulkanDescriptorHeap()
{
    vmaDestroyBuffer(m_allocator, m_heap.buffer, m_heap.allocation);
}

bool VulkanDescriptorHeap::create(const DescriptorHeapDesc& desc, const VulkanDescriptorHeapInitInfo& init_info)
{
    const auto& heap_info = init_info.heap_info;
    const auto size = desc.num_descriptors * heap_info.descriptor_size + heap_info.reserved_size;

    if (size > heap_info.max_size)
    {
        return false;
    }

    VkBufferCreateInfo buffer_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    };

    VmaAllocationCreateInfo allocation_create_info{
        // TODO: Don't do this, quick fix to allow creating into GPU heaps directly
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };
    if (desc.visibility == DescriptorHeapDesc::Visibility::GPU)
    {
        allocation_create_info.requiredFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        buffer_info.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    else
    {
        {
            buffer_info.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }
    }

    VmaAllocationInfo alloc_info;
    if (VK_FAILED(vmaCreateBufferWithAlignment(init_info.allocator,
                                               &buffer_info,
                                               &allocation_create_info,
                                               heap_info.heap_alignment,
                                               &m_heap.buffer,
                                               &m_heap.allocation,
                                               &alloc_info)))
    {
        return false;
    }

    m_data = alloc_info.pMappedData;
    assert(m_data);

    m_allocator = init_info.allocator;

    m_total_size = size;
    m_reserved_size = init_info.heap_info.reserved_size;

    return true;
}

void* VulkanDescriptorHeap::get_cpu_pointer(const size_t offset) const
{
    return static_cast<uint8_t*>(m_data) + m_reserved_size + offset;
}

VkBuffer VulkanDescriptorHeap::get_buffer() const
{
    return m_heap.buffer;
}

VkDeviceSize VulkanDescriptorHeap::get_total_size() const
{
    return m_total_size;
}
