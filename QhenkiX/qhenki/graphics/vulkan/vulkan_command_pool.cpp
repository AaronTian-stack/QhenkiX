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
    vkFreeCommandBuffers(device, command_pool, command_buffers.size(), command_buffers.data());
    if (command_pool)
    {
        vkDestroyCommandPool(device, command_pool, nullptr);
    }
}

VkCommandBuffer VulkanCommandPool::create_command_buffer(VkCommandBufferAllocateInfo& info)
{
    assert(command_pool);
    info.commandPool = command_pool;
    command_buffers.emplace_back();
    if (vkAllocateCommandBuffers(device, &info, &command_buffers.back()) != VK_SUCCESS)
    {
        command_buffers.pop_back();
        return VK_NULL_HANDLE;
    }
    return command_buffers.back();
}

VkResult VulkanCommandPool::reset()
{
    assert(command_pool);
    vkFreeCommandBuffers(device, command_pool, command_buffers.size(), command_buffers.data());
    command_buffers.clear();
    return vkResetCommandPool(device, command_pool, 0);
}
