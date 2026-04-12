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
    if (current_command_buffer_index > 0)
    {
        vkFreeCommandBuffers(device, command_pool, current_command_buffer_index + 1, command_buffers.data());
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
    if (current_command_buffer_index >= command_buffers.size() ||
        vkAllocateCommandBuffers(device, &info, &buffer) != VK_SUCCESS)
    {
        return VK_NULL_HANDLE;
    }
    command_buffers[current_command_buffer_index++] = buffer;
    return buffer;
}

VkResult VulkanCommandPool::reset()
{
    assert(command_pool);
    if (current_command_buffer_index > 0)
    {
        vkFreeCommandBuffers(device, command_pool, current_command_buffer_index + 1, command_buffers.data());
    }
    current_command_buffer_index = 0;
    return vkResetCommandPool(device, command_pool, 0);
}
