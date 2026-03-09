#include "vulkan_descriptor_heap.h"

using namespace qhenki::gfx;

VulkanDescriptorHeap::~VulkanDescriptorHeap()
{
    vmaDestroyVirtualBlock(m_block);
    vmaDestroyBuffer(m_context->m_allocator, m_heap.buffer, m_heap.allocation);
}

bool VulkanDescriptorHeap::create(const DescriptorHeapDesc& desc, const VulkanContext& context)
{
    const auto& properties = context.m_capabilities.descriptor_heap_properties;

    m_reserved_size = desc.type == DescriptorHeapDesc::Type::SAMPLER ? properties.minSamplerHeapReservedRange
                                                                     : properties.minResourceHeapReservedRange;

    const auto size = desc.size + m_reserved_size;

    assert(desc.type == DescriptorHeapDesc::Type::SAMPLER ? size < properties.maxSamplerHeapSize
                                                          : size < properties.maxResourceHeapSize);

    const VkBufferCreateInfo buffer_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };

    const VmaAllocationCreateInfo alloc_info{
        .usage = VMA_MEMORY_USAGE_AUTO,
    };
    // TODO: Need VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;?

    auto result =
        vmaCreateBuffer(context.m_allocator, &buffer_info, &alloc_info, &m_heap.buffer, &m_heap.allocation, nullptr);

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

    if (m_reserved_size > 0)
    {
        VmaVirtualAllocation reserved_alloc = VK_NULL_HANDLE;
        VkDeviceSize reserved_offset = 0;
        const VmaVirtualAllocationCreateInfo reserved_info{
            .size = m_reserved_size,
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

    return true;
}

bool VulkanDescriptorHeap::allocate(VmaVirtualAllocation* va,
                                    const Descriptor::Type type,
                                    const VulkanContext& context,
                                    VkDeviceSize* offset) const
{
    const auto& properties = context.m_capabilities.descriptor_heap_properties;

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

    return res == VK_SUCCESS;
}

void VulkanDescriptorHeap::deallocate(const VmaVirtualAllocation va) const
{
    vmaVirtualFree(m_block, va);
}

VkDeviceAddress VulkanDescriptorHeap::get_address() const
{
    const VkBufferDeviceAddressInfo addr_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .buffer = m_heap.buffer,
    };
    return vkGetBufferDeviceAddress(m_context->m_device, &addr_info);
}

VkDeviceSize VulkanDescriptorHeap::get_reserved_size() const
{
    return m_reserved_size;
}
