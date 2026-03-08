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
    vkFreeCommandBuffers(device, command_pool, command_buffers.size(), command_buffers.data());
    if (command_pool)
    {
        vkDestroyCommandPool(device, command_pool, nullptr);
    }
}

VkCommandBuffer VulkanCommandPool::create_command_buffer(VkCommandBufferAllocateInfo& info)
{
    assert(device);
    assert(command_pool);
    info.commandPool = command_pool;
    info.commandBufferCount = 1;
    VkCommandBuffer buffer = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(device, &info, &buffer) != VK_SUCCESS)
    {
        return VK_NULL_HANDLE;
    }
    command_buffers.push_back(buffer);
    return buffer;
}

VkResult VulkanCommandPool::reset()
{
    assert(command_pool);
    vkFreeCommandBuffers(device, command_pool, command_buffers.size(), command_buffers.data());
    command_buffers.clear();
    return vkResetCommandPool(device, command_pool, 0);
}
