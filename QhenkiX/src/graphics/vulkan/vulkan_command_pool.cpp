#include "vulkan_command_pool.h"

#include <cassert>

using namespace qhenki::gfx;

VulkanCommandPool::VulkanCommandPool(const VkDevice device, const VkCommandPool pool)
    : device(device),
      command_pool(pool)
{
}

VulkanCommandPool::~VulkanCommandPool()
{
    assert(device);
    if (command_buffer_count > 0)
    {
        vkFreeCommandBuffers(device, command_pool, command_buffer_count + 1, command_buffers.data());
    }
    if (command_pool)
    {
        vkDestroyCommandPool(device, command_pool, nullptr);
    }
}

void VulkanCommandPool::init(const VkDevice device, const VkCommandPool pool)
{
    this->device = device;
    this->command_pool = pool;
}

VkCommandBuffer VulkanCommandPool::create_command_buffer(VkCommandBufferAllocateInfo& info)
{
    assert(device);
    assert(command_pool);
    info.commandPool = command_pool;
    info.commandBufferCount = 1;
    VkCommandBuffer buffer = VK_NULL_HANDLE;
    if (command_buffer_count >= command_buffers.size() ||
        vkAllocateCommandBuffers(device, &info, &buffer) != VK_SUCCESS)
    {
        return VK_NULL_HANDLE;
    }
    command_buffers[command_buffer_count++] = buffer;
    return buffer;
}

VkResult VulkanCommandPool::reset()
{
    assert(command_pool);
    if (command_buffer_count > 0)
    {
        vkFreeCommandBuffers(device, command_pool, command_buffer_count, command_buffers.data());
    }
    command_buffer_count = 0;
    return vkResetCommandPool(device, command_pool, 0);
}
