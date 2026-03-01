#include "vulkan_descriptor_heap.h"

using namespace qhenki::gfx;

bool VulkanDescriptorHeap::create(const DescriptorHeapDesc& desc, const VulkanContext& context)
{
    const auto& properties = context.m_capabilities.descriptor_heap_properties;

    assert(desc.type == DescriptorHeapDesc::Type::SAMPLER ? desc.size < properties.maxSamplerHeapSize
                                                          : desc.size < properties.maxResourceHeapSize);

    const VkBufferCreateInfo buffer_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = desc.size,
        .usage = VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    };

    const VmaAllocationCreateInfo alloc_info{
        .usage = VMA_MEMORY_USAGE_AUTO,
    };
    // TODO: Need VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;?

    auto result =
        vmaCreateBuffer(context.m_allocator, &buffer_info, &alloc_info, &m_heap.buffer, &m_heap.allocation, nullptr);

    if (result != VK_SUCCESS)
    {
        return false;
    }

    const VmaVirtualBlockCreateInfo block_create_info{
        .size = desc.size,
    };

    result = vmaCreateVirtualBlock(&block_create_info, &m_block);

    return result == VK_SUCCESS;
}

bool VulkanDescriptorHeap::allocate(VmaVirtualAllocation* va,
                                    const Descriptor::Type type,
                                    const VulkanContext& context) const
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

    VkDeviceSize offset;
    const auto res = vmaVirtualAllocate(m_block, &alloc_create_info, va, &offset);

    return res == VK_SUCCESS;
}

void VulkanDescriptorHeap::deallocate(const VmaVirtualAllocation va) const
{
    vmaVirtualFree(m_block, va);
}
