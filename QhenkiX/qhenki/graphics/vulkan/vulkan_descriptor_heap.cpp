#include "vulkan_descriptor_heap.h"

#include <vk_mem_alloc.h>

using namespace qhenki::gfx;

VulkanDescriptorHeap::~VulkanDescriptorHeap()
{
    vmaDestroyVirtualBlock(m_block);
    vmaDestroyBuffer(m_context->m_allocator, m_heap.buffer, m_heap.allocation);
}

bool VulkanDescriptorHeap::create(const DescriptorHeapDesc& desc, const VulkanContext& context)
{
    const auto& properties = context.m_capabilities.descriptor_heap_properties;

    auto reserved_size = desc.type == DescriptorHeapDesc::Type::SAMPLER ? properties.minSamplerHeapReservedRange
                                                                        : properties.minResourceHeapReservedRange;

    const auto size = desc.size + reserved_size;

    if (!(desc.type == DescriptorHeapDesc::Type::SAMPLER ? size < properties.maxSamplerHeapSize
                                                         : size < properties.maxResourceHeapSize))
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
    auto result = vmaCreateBuffer(
        context.m_allocator, &buffer_info, &allocation_create_info, &m_heap.buffer, &m_heap.allocation, &alloc_info);

    if (VK_FAILED(result))
    {
        return false;
    }

    // Immediately allocate prefix
    const VmaVirtualBlockCreateInfo block_create_info{
        .size = size,
    };

    result = vmaCreateVirtualBlock(&block_create_info, &m_block);

    if (VK_FAILED(result))
    {
        return false;
    }

    if (reserved_size > 0)
    {
        VmaVirtualAllocation reserved_alloc = VK_NULL_HANDLE;
        VkDeviceSize reserved_offset = 0;
        const VmaVirtualAllocationCreateInfo reserved_info{
            .size = reserved_size,
            .alignment = 1,
        };

        result = vmaVirtualAllocate(m_block, &reserved_info, &reserved_alloc, &reserved_offset);

        if (VK_FAILED(result) || reserved_offset != 0)
        {
            vmaDestroyVirtualBlock(m_block);
            m_block = VK_NULL_HANDLE;
            return false;
        }
    }

    m_data = alloc_info.pMappedData;
    assert(m_data);

    m_context = &context;
    m_total_size = size;
    m_reserved_size = reserved_size;

    return true;
}

bool VulkanDescriptorHeap::allocate(VmaVirtualAllocation* va, const Descriptor::Type type, VkDeviceSize* offset) const
{
    const auto& properties = m_context->m_capabilities.descriptor_heap_properties;

    VmaVirtualAllocationCreateInfo alloc_create_info = {};

    switch (type)
    {
    case Descriptor::BUFFER:
        alloc_create_info.size = properties.bufferDescriptorSize;
        alloc_create_info.alignment = properties.bufferDescriptorAlignment;
        break;
    case Descriptor::TEXTURE:
        alloc_create_info.size = properties.imageDescriptorSize;
        alloc_create_info.alignment = properties.imageDescriptorAlignment;
        break;
    case Descriptor::SAMPLER:
        alloc_create_info.size = properties.samplerDescriptorSize;
        alloc_create_info.alignment = properties.samplerDescriptorAlignment;
        break;
    default:
        return false;
    }

    const auto res = vmaVirtualAllocate(m_block, &alloc_create_info, va, offset);
    if (res == VK_SUCCESS)
    {
        *offset -= m_reserved_size;
    }

    return res == VK_SUCCESS;
}

void VulkanDescriptorHeap::deallocate(const VmaVirtualAllocation va) const
{
    assert(va);
    vmaVirtualFree(m_block, va);
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

VkDeviceSize VulkanDescriptorHeap::get_reserved_size() const
{
    return m_reserved_size;
}

VmaVirtualAllocationInfo VulkanDescriptorHeap::get_allocation_info(const VmaVirtualAllocation va) const
{
    VmaVirtualAllocationInfo info;
    vmaGetVirtualAllocationInfo(m_block, va, &info);
    return info;
}
