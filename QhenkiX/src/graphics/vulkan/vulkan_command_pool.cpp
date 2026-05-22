#include "vulkan_command_pool.h"

#include <cassert>

using namespace qhenki::gfx;

VulkanCommandPool::VulkanCommandPool(const VkDevice device, const VkCommandPool pool)
    : m_device(device),
      m_command_pool(pool)
{
}

VulkanCommandPool::~VulkanCommandPool()
{
    assert(m_device);
    if (m_command_buffer_count > 0)
    {
        vkFreeCommandBuffers(m_device, m_command_pool, m_command_buffer_count + 1, m_command_buffers.data());
    }
    if (m_command_pool)
    {
        vkDestroyCommandPool(m_device, m_command_pool, nullptr);
    }
}

void VulkanCommandPool::init(const VkDevice device, const VkCommandPool pool)
{
    this->m_device = device;
    this->m_command_pool = pool;
}

bool VulkanCommandPool::is_valid() const
{
    return m_device != VK_NULL_HANDLE && m_command_pool != VK_NULL_HANDLE;
}

VkCommandBuffer VulkanCommandPool::create_command_buffer(VkCommandBufferAllocateInfo& info)
{
    assert(m_device);
    assert(m_command_pool);
    info.commandPool = m_command_pool;
    info.commandBufferCount = 1;
    VkCommandBuffer buffer = VK_NULL_HANDLE;
    if (m_command_buffer_count >= m_command_buffers.size() ||
        vkAllocateCommandBuffers(m_device, &info, &buffer) != VK_SUCCESS)
    {
        return VK_NULL_HANDLE;
    }
    m_command_buffers[m_command_buffer_count++] = buffer;
    return buffer;
}

VkResult VulkanCommandPool::reset()
{
    assert(m_command_pool);
    if (m_command_buffer_count > 0)
    {
        vkFreeCommandBuffers(m_device, m_command_pool, m_command_buffer_count, m_command_buffers.data());
    }
    m_command_buffer_count = 0;
    return vkResetCommandPool(m_device, m_command_pool, 0);
}
